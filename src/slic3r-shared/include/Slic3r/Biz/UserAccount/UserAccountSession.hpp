#pragma once

#include "Slic3r/Biz/UserAccount/UserAccountActionStore.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountSessionDispatchBase.hpp"

#include <queue>
#include <mutex>
#include <deque>

namespace Slic3r::Biz::UserAccount {
/**
 * @brief Queue of UserAccountAction in own thread.
 * Receives and queues requests of Actions.
 * Performs Actions including handling its results (f.e. resolving tokens)
 * Passes data outside via IMainThreadDispatcher implemented in UserAccountSessionDispatchBase. 
 */
class UserAccountSession final : public UserAccountSessionDispatchBase {
public:
	UserAccountSession(Platform::IMainThreadDispatcher& dispatcher);
	~UserAccountSession() = default;

	UserAccountSession(const UserAccountSession& ) = delete;
    UserAccountSession(UserAccountSession&& other) = delete;
    UserAccountSession& operator=(const UserAccountSession& ) = delete;
    UserAccountSession& operator=(UserAccountSession&& other) = delete;
    

    void set_tokens(const std::string& access_token, const std::string& refresh_token, const std::string& shared_session_key, long long expires_in);

    /**
     * @brief Clears all data, logs out.
    */
    void do_clear(bool notify_owner);

    /**
     * @brief One by one processes whole action queue.
     */
    void process_action_queue();

    /**
     * @brief Enqueues CodeForToken action with callbacks.
     */
    void on_log_in_code_response(const std::string& code, const std::string& code_verifier);
    void enqueue_action(UserAccountActionID id, ActionSuccessFn success_callback, ActionFailFn fail_callback, const std::string& input);
    void enqueue_test_with_refresh();
    void enqueue_refresh(const std::string& body);
    void enqueue_refresh_race(const std::string& refresh_token_from_store);

    bool is_enqueued(UserAccountActionID action_id) const;
   

    bool is_initialized() const {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        return !m_access_token.empty() || !m_refresh_token.empty();
    }
    std::string get_access_token() const {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        return m_access_token;
    }
    std::string get_refresh_token() const {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        return m_refresh_token;
    }
    std::string get_shared_session_key() const {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        return m_shared_session_key;
    }
    long long get_next_token_timeout() const {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        return m_next_token_timeout;
    }


private:
	mutable std::mutex m_session_mutex;
    // guarded by m_session_mutex

    UserAccountActionStore      m_actions;
    std::queue<ActionQueueData> m_action_queue;
    std::deque<ActionQueueData> m_priority_action_queue; 
    /**
     * @brief Prevents action queue to be processed if false - no communication is done
     *  sets to true by on_log_in_code_response or enqueue_action call
     */
    bool m_processing_enabled {false}; 
    
    void enqueue_action_inner(UserAccountActionID id, ActionSuccessFn success_callback, ActionFailFn fail_callback, const std::string& input);

    // End of section guarded by m_session_mutex

    mutable std::mutex m_credentials_mutex;
    // guarded by m_credentials_mutex
    std::string m_access_token;
    std::string m_refresh_token;
    std::string m_shared_session_key;
    long long m_next_token_timeout { 0 };
    // End of section guarded by m_credentials_mutex

    std::atomic_bool m_global_cancel {false};
    /**
     * Called to pass data from Session thread to UI thread.
     */

    void refresh_fail_callback(const std::string& body);
    void refresh_fail_soft_callback(const std::string& body);
    void cancel_queue();
    void code_exchange_fail_callback(const std::string& body);
    void token_success_callback(const std::string& body);
    void process_action_queue_inner();
    void remove_from_queue(UserAccountActionID action_id);
};
}