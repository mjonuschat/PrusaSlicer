#pragma once

#include <Slic3r/Biz/PrintHost/IPrintHostListener.hpp>
#include <Slic3r/Biz/PrintHost/IPrintHost.hpp>
#include <Slic3r/Biz/PrintHost/IPrintHostJobCallbacks.hpp>
#include <Slic3r/Biz/PrintHost/PrintHostConfig.hpp>

#include <jthread/JThread.hpp>
#include <libassert/assert.hpp>
#include <memory>
#include <functional>
#include <future>

namespace Slic3r::Biz::PrintHost {


class PrintHostJob
{
public:
    PrintHostJob(IPrintHostJobCallbacks* owner, size_t id, PrintHostConfig config, PrintHostJobData data);

    ~PrintHostJob();

    /**
     * @brief Starts the job in m_thread.
     */
    void start();

    /**
     * @brief Requests the job to stop.
     */
    void cancel();

    /**
     * @brief Returns host from config. Used from outside PrintHostJob for setting futures.
     */
    std::string get_host() const { return m_host; }

    /**
     * @brief Called from inside m_thread to wait for other jobs with same host to finish.
     */
    std::shared_future<void> get_future();

    /**
     * @brief Stores pointer to other Job to call get_future() on it.
     */
    void add_dependency(std::shared_ptr<PrintHostJob> dependency);
private:
    JThread::JThread m_thread;
    
    std::unique_ptr<IPrintHost> m_print_host;
    PrintHostJobData m_upload_data;
    size_t m_id;

    /**
     * @brief Pointer to owner of this job. Used for callbacks.
     */
    IPrintHostJobCallbacks* m_owner;

    /**
     * Host is stored separately, always being readable from outside PrintHostJob.
     */
    std::string m_host;

    /**
     * @brief Callback set od PrintHost perform function. Passes progress to owner.
     */
    void on_progress_fn(Network::IHttp::Progress&& progress, bool &cancel);

    /**
     * @brief Callback set od PrintHost perform function. Passes error to owner.
     */
    void on_error_fn(std::string&& error);

    /**
     * @brief Callback set od PrintHost perform function. Passes info to owner.
     */
    void on_info_fn(std::string&& tag, std::string&& host);

    /**
     * @brief Callback set od PrintHost perform function. Passes retry to owner.
     */
    void on_retry_fn(Network::IHttp::Retry&& retry, bool&cancel);

    /**
     * Job wont start until all dependencies are done
     */
    std::vector<std::shared_ptr<PrintHostJob>> m_dependencies;

    /**
     * Promise is set when job is done.
     */
    std::promise<void> m_promise;

    /**
     * Shared future from m_promise. Set in constructor. Later shared with other jobs. More than one job might be dependent on this job. Thus shared_future.
     */
    std::shared_future<void> m_future;

};

} // namespace Slic3r::Biz::PrintHost
