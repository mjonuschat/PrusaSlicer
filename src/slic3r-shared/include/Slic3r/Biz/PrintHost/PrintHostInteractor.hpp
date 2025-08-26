#pragma once

#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostJobManager.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostDataFinalizer.hpp"

#include <boost/filesystem.hpp>
#include <map>

namespace Slic3r::Biz::PrintHost {

class PrintHostJobManager;

class PrintHostInteractor : public IPrintHostBinarizeListener
{
public:
    PrintHostInteractor(Platform::IMainThreadDispatcher& dispatcher);

    typedef std::function<void(const std::string& storage_json)> StorageInfoFn;

    void on_storage_resolved(size_t id, const std::string& storage);

    /**
     * @brief Accepts data from PrintHostJobManager and passes it to PrintHostDataFinalizer and process_gcode_inner.
     */
    void export_gcode(PrintHostConfig config, PrintHostJobData data);

    /**
     * @brief Accepts data from PrintHostJobManager and passes it to PrintHostDataFinalizer and process_gcode_inner.
     */
    void upload_gcode(PrintHostConfig config, PrintHostJobData data);

    void on_print_host_binarize_success(PrintHostConfig config, PrintHostJobData data) override;
    void on_print_host_binarize_fail(const std::string& msg) override;

private:
    PrintHostJobManager m_print_host_job_manager;
    PrintHostDataFinalizer m_print_host_data_finalizer;
    std::map<size_t, StorageInfoFn> m_storage_callbacks_map;

    /**
     * @brief Passes data to PrintHostJobManager to first start storage resolve job and stages upload job data to m_storage_callbacks_map.
     */
    void upload_gcode_with_storage_choice(PrintHostConfig config, PrintHostJobData data);

    /**
     * @brief Passes data to PrintHostJobManager to create and start export / upload job.
     */
    void process_gcode_inner(PrintHostConfig config, PrintHostJobData data);
};
} // namespace Slic3r::Biz::PrintHost
