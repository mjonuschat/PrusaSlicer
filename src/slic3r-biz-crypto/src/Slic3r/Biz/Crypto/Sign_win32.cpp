#include "Slic3r/Biz/Crypto/Sign.hpp"
#include "Slic3r/Assert.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <vector>
#include <stdexcept>
#include <fmt/format.h>

namespace Slic3r::Biz::Crypto {

namespace Internal {
struct KeyImpl
{
    BCRYPT_KEY_HANDLE handle_key{nullptr};
    BCRYPT_ALG_HANDLE handle_alg{nullptr};

    KeyImpl() = default;

    ~KeyImpl()
    {
        if (handle_key)
            BCryptDestroyKey(handle_key);
        if (handle_alg)
            BCryptCloseAlgorithmProvider(handle_alg, 0);
    }

    // Move semantics required
    KeyImpl(KeyImpl&& other) noexcept
    {
        handle_key       = other.handle_key;
        other.handle_key = nullptr;
        handle_alg       = other.handle_alg;
        other.handle_alg = nullptr;
    }

    KeyImpl& operator=(KeyImpl&& other) noexcept
    {
        if (this != &other) {
            if (handle_key)
                BCryptDestroyKey(handle_key);
            if (handle_alg)
                BCryptCloseAlgorithmProvider(handle_alg, 0);
            handle_key       = other.handle_key;
            other.handle_key = nullptr;
            handle_alg       = other.handle_alg;
            other.handle_alg = nullptr;
        }
        return *this;
    }
};
} // namespace Internal

KeyPair::KeyPair() : m_key(std::make_unique<Internal::KeyImpl>()) {}

KeyPair::~KeyPair() = default;

KeyPair::KeyPair(KeyPair&&) noexcept            = default;
KeyPair& KeyPair::operator=(KeyPair&&) noexcept = default;

KeyPair::KeyPair(ImplPtr&& impl) : m_key(std::move(impl)) {}

KeyPair KeyPair::generate(const char* algo, int size)
{
    auto impl      = std::make_unique<Internal::KeyImpl>();
    LPCWSTR alg_id = (std::string(algo) == "RSA") ?
        BCRYPT_RSA_ALGORITHM :
        BCRYPT_ECDSA_P256_ALGORITHM; // Fallback to handle generic cases

    NTSTATUS status = BCryptOpenAlgorithmProvider(&impl->handle_alg, alg_id, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        throw CryptoException(fmt::format("Unsupported keygen cypher {}", algo));
    }

    status = BCryptGenerateKeyPair(impl->handle_alg, &impl->handle_key, size, 0);
    if (BCRYPT_SUCCESS(status)) {
        BCryptFinalizeKeyPair(impl->handle_key, 0);
    }
    return KeyPair{std::move(impl)};
}

KeyPair KeyPair::generate(int size)
{
    return generate("RSA", size);
}

KeyPair KeyPair::load_pub_pem(BytesView bytes)
{
    DWORD der_len = 0;
    if (!CryptStringToBinaryA(
            reinterpret_cast<LPCSTR>(bytes.data()),
            static_cast<DWORD>(bytes.size()),
            CRYPT_STRING_BASE64HEADER,
            nullptr,
            &der_len,
            nullptr,
            nullptr
        ))
    {
        throw CryptoException("Reading PEM failed: CryptStringToBinaryA");
    }

    std::vector<BYTE> der(der_len);
    CryptStringToBinaryA(
        reinterpret_cast<LPCSTR>(bytes.data()),
        static_cast<DWORD>(bytes.size()),
        CRYPT_STRING_BASE64HEADER,
        der.data(),
        &der_len,
        nullptr,
        nullptr
    );

    void* info_ptr = nullptr;
    DWORD info_len = 0;
    if (!CryptDecodeObjectEx(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            X509_PUBLIC_KEY_INFO,
            der.data(),
            der_len,
            CRYPT_DECODE_ALLOC_FLAG,
            nullptr,
            &info_ptr,
            &info_len
        ))
    {
        throw CryptoException("Reading PEM failed: CryptDecodeObjectEx");
    }

    auto impl    = std::make_unique<Internal::KeyImpl>();
    bool success = CryptImportPublicKeyInfoEx2(
        X509_ASN_ENCODING,
        reinterpret_cast<PCERT_PUBLIC_KEY_INFO>(info_ptr),
        0,
        nullptr,
        &impl->handle_key
    );
    LocalFree(info_ptr);

    if (!success) {
        throw CryptoException("Reading PEM failed: CryptImportPublicKeyInfoEx2");
    }

    return KeyPair{std::move(impl)};
}

// -----------------------------------------------------------------------------
// Core logic for CNG Builders and Verifiers
// -----------------------------------------------------------------------------

LPCWSTR get_bcrypt_hash_algo(HashType type)
{
    switch (type) {
    case HashType::SHA_1:
        return BCRYPT_SHA1_ALGORITHM;
    case HashType::SHA_256:
        return BCRYPT_SHA256_ALGORITHM;
    case HashType::SHA_512:
        return BCRYPT_SHA512_ALGORITHM;
    default:
        return nullptr;
    }
}

class CngSignatureImpl
{
protected:
    BCRYPT_ALG_HANDLE m_handle_hash_alg{nullptr};
    BCRYPT_HASH_HANDLE m_handle_hash{nullptr};
    LPCWSTR m_hash_algo_str;
    const KeyPair& m_key;
    bool m_failed{false};

    CngSignatureImpl(HashType hash_type, const KeyPair& key) : m_key(key)
    {
        m_hash_algo_str = get_bcrypt_hash_algo(hash_type);
        if (!m_hash_algo_str
            || !BCRYPT_SUCCESS(
                BCryptOpenAlgorithmProvider(&m_handle_hash_alg, m_hash_algo_str, nullptr, 0)
            ))
        {
            m_failed = true;
            return;
        }
        if (!BCRYPT_SUCCESS(
                BCryptCreateHash(m_handle_hash_alg, &m_handle_hash, nullptr, 0, nullptr, 0, 0)
            ))
        {
            m_failed = true;
        }
    }

    ~CngSignatureImpl()
    {
        if (m_handle_hash)
            BCryptDestroyHash(m_handle_hash);
        if (m_handle_hash_alg)
            BCryptCloseAlgorithmProvider(m_handle_hash_alg, 0);
    }

    void update_hash(BytesView bytes)
    {
        if (!m_failed && !bytes.empty()) {
            if (!BCRYPT_SUCCESS(
                    BCryptHashData(m_handle_hash, (PUCHAR) bytes.data(), (ULONG) bytes.size(), 0)
                ))
            {
                m_failed = true;
            }
        }
    }

    Bytes finalize_hash()
    {
        DWORD hash_size = 0;
        DWORD cb_data   = 0;
        BCryptGetProperty(
            m_handle_hash_alg,
            BCRYPT_HASH_LENGTH,
            (PUCHAR) &hash_size,
            sizeof(hash_size),
            &cb_data,
            0
        );

        Bytes hash(hash_size);
        if (!m_failed
            && BCRYPT_SUCCESS(BCryptFinishHash(m_handle_hash, (PUCHAR) hash.data(), hash_size, 0)))
        {
            return hash;
        }
        m_failed = true;
        return {};
    }
};

class CngSignatureBuilder : public ISignatureBuilder, public CngSignatureImpl
{
public:
    CngSignatureBuilder(HashType hash_type, const KeyPair& key) : CngSignatureImpl(hash_type, key)
    {}

    void update(BytesView bytes) override
    {
        update_hash(bytes);
    }

    Signature finalize() override
    {
        Bytes hash_data = finalize_hash();
        if (m_failed)
            return Signature{};

        BCRYPT_PKCS1_PADDING_INFO padding_info = {m_hash_algo_str};
        DWORD sig_len                          = 0;

        if (!BCRYPT_SUCCESS(BCryptSignHash(
                m_key.internal_impl().handle_key,
                &padding_info,
                (PUCHAR) hash_data.data(),
                (ULONG) hash_data.size(),
                nullptr,
                0,
                &sig_len,
                BCRYPT_PAD_PKCS1
            )))
        {
            return Signature{};
        }

        Bytes signature(sig_len);
        if (!BCRYPT_SUCCESS(BCryptSignHash(
                m_key.internal_impl().handle_key,
                &padding_info,
                (PUCHAR) hash_data.data(),
                (ULONG) hash_data.size(),
                (PUCHAR) signature.data(),
                sig_len,
                &sig_len,
                BCRYPT_PAD_PKCS1
            )))
        {
            return Signature{};
        }

        return Signature{std::move(signature)};
    }
};

class CngSignatureVerifier : public ISignatureVerifier, public CngSignatureImpl
{
public:
    CngSignatureVerifier(HashType hash_type, const KeyPair& key) : CngSignatureImpl(hash_type, key)
    {}

    void update(BytesView bytes) override
    {
        update_hash(bytes);
    }

    bool finalize(const Signature& signature) override
    {
        Bytes hash_data = finalize_hash();
        if (m_failed)
            return false;

        BCRYPT_PKCS1_PADDING_INFO padding_info = {m_hash_algo_str};

        NTSTATUS status = BCryptVerifySignature(
            m_key.internal_impl().handle_key,
            &padding_info,
            (PUCHAR) hash_data.data(),
            (ULONG) hash_data.size(),
            (PUCHAR) signature.bytes().data(),
            (ULONG) signature.bytes().size(),
            BCRYPT_PAD_PKCS1
        );

        return BCRYPT_SUCCESS(status);
    }
};

ISignatureBuilderPtr create_signature_builder(HashType hash_type, const KeyPair& key_pair)
{
    return std::make_unique<CngSignatureBuilder>(hash_type, key_pair);
}

ISignatureVerifierPtr create_signature_verifier(HashType hash_type, const KeyPair& key_pair)
{
    return std::make_unique<CngSignatureVerifier>(hash_type, key_pair);
}

} // namespace Slic3r::Biz::Crypto
