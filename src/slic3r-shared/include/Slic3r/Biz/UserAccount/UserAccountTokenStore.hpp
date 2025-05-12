#pragma once

#include <string>

namespace Slic3r::Biz::UserAccount::TokenStore {

struct StoreData {
    std::string access_token;
    std::string refresh_token;
    std::string shared_session_key;
    std::string next_timeout;
    std::string master_pid;
};

/**
 * @brief Saves secret to store.
 * @return true if secret was saved successfully.
 */
bool save_tokens(const StoreData& secrets);

/**
 * @brief Loads secret from store.
 * @return true if secret was loaded successfully.
 */
bool load_tokens(StoreData& result);

}