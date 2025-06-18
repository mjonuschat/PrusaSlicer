#include "Slic3r/Biz/WX/FontManager.hpp"
#include "Slic3r/Biz/WX/FontUtils.hpp"

#include <wx/fontenum.h>
#include <boost/functional/hash.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/log/trivial.hpp>

// cache font list by cereal
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>

using namespace Slic3r;

template<class Archive>
void save(Archive& archive, wxString const& d)
{
    std::string s(d.ToUTF8().data());
    archive(s);
}

template<class Archive>
void load(Archive& archive, wxString& d)
{
    std::string s;
    archive(s);
    d = wxString::FromUTF8(s);
}

namespace {
// increase number when change struct FacenamesSerializer
constexpr std::uint32_t FACENAMES_VERSION = 1;

struct FacenamesSerializer
{
    // hash number for unsorted vector of installed font into system
    size_t hash = 0;
    // assumption that is loadable
    std::vector<wxString> good;
    // Can't load for some reason
    std::vector<wxString> bad;
};

template<class Archive>
void serialize(Archive& ar, FacenamesSerializer& t, const std::uint32_t version)
{
    // When performing a load, the version associated with the class
    // is whatever it was when that data was originally serialized
    // When we save, we'll use the version that is defined in the macro
    if (version != FACENAMES_VERSION)
        return;
    ar(t.hash, t.good, t.bad);
}

bool store(
    const boost::filesystem::path& path,
    const std::vector<wxString>& good,
    const std::vector<wxString>& bad,
    size_t hash
)
{
    boost::nowide::ofstream file(path, std::ios::binary);
    ::cereal::BinaryOutputArchive archive(file);
    FacenamesSerializer data{.hash = hash, .good = good, .bad = bad};
    assert(std::is_sorted(data.bad.begin(), data.bad.end()));
    assert(std::is_sorted(data.good.begin(), data.good.end()));

    try {
        archive(data);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Failed to write fontlist cache - " << path.string() << ex.what();
        return false;
    }
    return true;
}

bool load(
    const boost::filesystem::path& path,
    Domain::FontList& openable,
    std::vector<wxString>& bad,
    size_t& hash,
    const wxFontEncoding encoding
)
{
    if (!boost::filesystem::exists(path)) {
        BOOST_LOG_TRIVIAL(warning) << "Fontlist cache - '" << path.string() << "' does not exists.";
        return false;
    }
    boost::nowide::ifstream file(path, std::ios::binary);
    cereal::BinaryInputArchive archive(file);

    FacenamesSerializer data;
    try {
        archive(data);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to load fontlist cache - '"
            << path.string()
            << "'. Exception: "
            << ex.what();
        return false;
    }

    if (!std::is_sorted(data.bad.begin(), data.bad.end()))
        return false;
    if (!std::is_sorted(data.good.begin(), data.good.end()))
        return false;

    hash = data.hash;
    bad = data.bad;

    // convert data.good into Array to be able use validation function
    wxArrayString names;
    names.reserve(data.good.size());
    for (auto it = data.good.begin(); it != data.good.end(); ++it)
        names.push_back(*it);

    Biz::WX::validate_fonts(names, openable, bad, wxFontEncoding::wxFONTENCODING_SYSTEM);
    return true;
}
} // namespace

///////////
// global definition
/////////
CEREAL_CLASS_VERSION(::FacenamesSerializer, FACENAMES_VERSION); // register class version

static std::size_t hash_value(wxString const& s)
{
    boost::hash<std::string> hasher;
    return hasher(std::string(s.ToUTF8()));
}

namespace Slic3r::Biz::WX {

FontManager::FontManager(const std::string& data_dir):
    m_cache_path(boost::filesystem::path(data_dir) / "cache" / "fonts.cereal")
{}

const Domain::FontList& FontManager::get_fonts()
{
    wxFontEnumerator::InvalidateCache();

    // Configuration of font encoding
    const wxFontEncoding encoding = wxFontEncoding::wxFONTENCODING_SYSTEM;
    wxArrayString facenames = wxFontEnumerator::GetFacenames(encoding);

    // check if it is same as last time
    size_t hash = boost::hash_range(facenames.begin(), facenames.end());
    if (!m_hash.has_value()) {
        // Application first call need load data from permanent cache
        size_t cache_hash;
        if (load(m_cache_path, m_openable, m_bad, cache_hash, encoding)) {
            m_hash = cache_hash;
        }
    }
    if (m_hash.has_value() && *m_hash == hash) {
        // no new installed font -> skip validation
        return m_openable;
    }

    BOOST_LOG_TRIVIAL(info)
        << "Changed fontlist detected("
        << "prev_hash="
        << m_hash.value_or(0)
        << ", "
        << "curr_hash="
        << hash
        << ").";

    m_hash = hash;
    m_valid = validate_fonts(facenames, m_openable, m_bad, encoding);
    store(m_cache_path, m_valid, m_bad, hash);
    return m_openable;
}

std::unique_ptr<const Domain::FontFile> FontManager::open(const Domain::FontDescriptor& descriptor)
{
    if (descriptor.type != get_current_type())
        return nullptr;
    auto result = create_font_file(load_wxFont(descriptor.path));
    if (m_hash.has_value() && result == nullptr) { // extend m_bad and store it
        auto openable_it = std::find_if(
            m_openable.begin(),
            m_openable.end(),
            [&path = descriptor.path](const Domain::FontDescriptor& d) {
                return d.path.compare(path) == 0;
            }
        );
        if (openable_it != m_openable.end()) {
            size_t index = openable_it - m_openable.begin();
            auto valid_it = m_valid.begin() + index;
            m_bad.push_back(*valid_it);
            std::sort(m_bad.begin(), m_bad.end());
            m_openable.erase(openable_it);
            m_valid.erase(valid_it);
            store(m_cache_path, m_valid, m_bad, *m_hash);
        }
    }
    return result;
}

} // namespace Slic3r::Biz::WX
