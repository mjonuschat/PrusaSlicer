#pragma once

#include "Slic3r/Biz/PrintHost/IPrintHostListener.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostConfig.hpp"
#include "Slic3r/Biz/PrintHost/IPrintHostJobCallbacks.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostJob.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace Slic3r::Biz::PrintHost {

class PrintHostDoneListener : public IPrintHostListener {
public:
    typedef std::function<void(size_t)> DoneFn;
    PrintHostDoneListener(DoneFn done_fn) : m_done_fn(done_fn) {}
    void on_print_host_progress(size_t id, int progress) override {}
    void on_print_host_error(size_t id, const std::string& msg) override {}
    void on_print_host_cancel(size_t id) override {}
    void on_print_host_done(size_t id) override { m_done_fn(id); }
    void on_print_host_info(size_t id, const std::string& tag, const std::string& msg) override {}
private:
    DoneFn m_done_fn;
};

class PrintHostJobManager : public IPrintHostJobCallbacks , public WithListeners<IPrintHostListener>
{
public:
    PrintHostJobManager(Platform::IMainThreadDispatcher& dispatcher);
    ~PrintHostJobManager();

    PrintHostJobManager(const PrintHostJobManager &) = delete;
    PrintHostJobManager(PrintHostJobManager &&other) = delete;
    PrintHostJobManager& operator=(const PrintHostJobManager &) = delete;
    PrintHostJobManager& operator=(PrintHostJobManager &&other) = delete;

    /**
     * Creates PrintHostJob and starts it.
     * Returns id of the job.
     * Since both emplace_job and erase_job are called from main thread, no mutex is needed here.
     */
    size_t emplace_job(PrintHostConfig config, PrintHostJobData data);
    void cancel(size_t id);

    /**
     * @brief Callback called from PrintHostJob thread.
     * Implements m_dispatcher call in thread-safe manner.
     */
    void on_job_progress(size_t id, int progress) override;

    /**
     * @brief Callback called from PrintHostJob thread.
     * Implements m_dispatcher call in thread-safe manner.
     */
    void on_job_error(size_t id, const std::string& msg) override;

    /**
     * @brief Callback called from PrintHostJob thread.
     * Implements m_dispatcher call in thread-safe manner.
     */
    void on_job_cancel(size_t id) override;

    /**
     * @brief Callback called from PrintHostJob thread.
     * Implements m_dispatcher call in thread-safe manner.
     */
    void on_job_done(size_t id) override;

    /**
     * @brief Callback called from PrintHostJob thread.
     * Implements m_dispatcher call in thread-safe manner.
     */
    void on_job_info(size_t id, const std::string& tag, const std::string& msg) override;

    /**
     * Callback for m_done_listener to cleanup finished job.
     * Since both emplace_job and erase_job are called from main thread, no mutex is needed here.
     */
    void erase_job(size_t id);

private:
    mutable std::mutex m_dispatcher_mutex;
    Platform::IMainThreadDispatcher &m_dispatcher;
    std::map<size_t, std::shared_ptr<PrintHostJob>> m_job_map;
    std::unique_ptr<IPrintHostListener> m_done_listener;
};

} // namespace Slic3r::Biz::Slicing
