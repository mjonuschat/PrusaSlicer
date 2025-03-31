#pragma once

#include "Slic3r/Biz/PrintHost/IPrintHostListener.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostJobManager.hpp"

#include <boost/filesystem.hpp>
#include <map>

namespace Slic3r::Biz::PrintHost {

class PrintHostJobManager;

class PrintHostInteractor : public IPrintHostListener {
public:
    PrintHostInteractor(Platform::IMainThreadDispatcher& dispatcher);

    typedef std::function<void(const std::string& storage_json)> StorageInfoFn;

    /**
     * @brief Callback from PrintHostJobManager with progress from PrintHostJob worker thread.
     * Should Pass data to listeners via ProjectInteractor to UI.
     */
    void on_print_host_progress(size_t id, int progress) override;
    
    /**
     * @brief Callback from PrintHostJobManager with progress from PrintHostJob worker thread.
     * Should Pass data to listeners via ProjectInteractor to UI.
     */
    void on_print_host_error(size_t id, const std::string& msg) override;
    
    /**
     * @brief Callback from PrintHostJobManager with progress from PrintHostJob worker thread.
     * Should Pass data to listeners via ProjectInteractor to UI.
     */
    void on_print_host_cancel(size_t id) override;
    
    /**
     * @brief Callback from PrintHostJobManager with progress from PrintHostJob worker thread.
     * Should Pass data to listeners via ProjectInteractor to UI.
     */
    void on_print_host_done(size_t id) override;
    
    /**
     * @brief Callback from PrintHostJobManager with progress from PrintHostJob worker thread.
     * Should Pass data to listeners via ProjectInteractor to UI.
     */
    void on_print_host_info(size_t id, const std::string& tag, const std::string& msg) override;

    /**
     * @brief Passes data to PrintHostJobManager to create and start export job.
     */
    void export_gcode(PrintHostConfig config, PrintHostJobData data);

    /**
     * @brief Passes data to PrintHostJobManager to create and start upload job.
     */
    void upload_gcode(PrintHostConfig config, PrintHostJobData data);
    

private:
    PrintHostJobManager m_print_host_job_manager;
    std::map<size_t, StorageInfoFn> m_storage_callbacks_map;
    
    /**
     * @brief Passes data to PrintHostJobManager to first start storage resolve job and stages upload job data to m_storage_callbacks_map.
     */
    void upload_gcode_with_storage_choice(PrintHostConfig config, PrintHostJobData data);
};
} // namespace Slic3r::Biz::PrintHost