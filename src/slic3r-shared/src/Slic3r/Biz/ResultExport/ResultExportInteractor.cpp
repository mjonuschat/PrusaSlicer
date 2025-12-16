#include "Slic3r/Biz/ResultExport/ResultExportInteractor.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"

#include "libslic3r/format.hpp"

#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::ResultExport {

ResultExportInteractor::ResultExportInteractor(Platform::IMainThreadDispatcher& dispatcher) :
    m_result_export_data_finalizer(dispatcher)
{
    m_result_export_data_finalizer.add_listener<IResultExportBinarizeListener>(this);
}

void ResultExportInteractor::on_storage_resolved(size_t id, const std::string& storage)
{
    if (auto node = m_storage_callbacks_map.extract(id); !node.empty()) {
        node.mapped()(storage);
    }
}

void ResultExportInteractor::perform(PrintHost::PrintHostConfig config, PrintHost::PrintHostJobData data)
{
    m_result_export_data_finalizer.finalize(std::move(config), std::move(data));
}

void ResultExportInteractor::process_gcode_inner(PrintHost::PrintHostConfig config, PrintHost::PrintHostJobData data)
{
    if (config.type == Domain::PrintHostType::PrusaLink) {
        upload_gcode_with_storage_choice(std::move(config), std::move(data));
        return;
    }
    size_t id = m_print_host_job_manager.emplace_job(std::move(config), std::move(data));
}

void ResultExportInteractor::upload_gcode_with_storage_choice(PrintHost::PrintHostConfig config, PrintHost::PrintHostJobData data)
{
    // Since copy constructor and assign are deleted for PrintHostConfig and PrintHostJobData,
    // We use shared pointer to be able to move them to the callback lambda and copy the lambda
    auto config_ptr = std::make_shared<PrintHost::PrintHostConfig>(std::move(config));
    auto data_ptr   = std::make_shared<PrintHost::PrintHostJobData>(std::move(data));

    StorageInfoFn callback = [this, config_ptr, data_ptr](const std::string& storage) mutable
    {
        data_ptr->storage = storage;
        m_print_host_job_manager.emplace_job(std::move(*config_ptr), std::move(*data_ptr));
    };

    // Create PrusaLinkStorage config by copying the shared pointer.
    // PrintHostConfig has deleted copy constructor.
    PrintHost::PrintHostConfig storage_config = {Domain::PrintHostType::PrusaLinkStorage, config_ptr->host};
    storage_config.ca_file         = config_ptr->ca_file;
    storage_config.ssl_revoke_best_effort = config_ptr->ssl_revoke_best_effort;
    storage_config.port                   = config_ptr->port;
    storage_config.auth_type              = config_ptr->auth_type;
    storage_config.api_key                = config_ptr->api_key;
    storage_config.username               = config_ptr->username;
    storage_config.password               = config_ptr->password;

    size_t id = m_print_host_job_manager.emplace_job(std::move(storage_config), {std::monostate{}, data_ptr->dest_path, PrintHost::PrintHostExportFormat::Undefined});

    m_storage_callbacks_map[id] = std::move(callback);
}

void ResultExportInteractor::on_result_export_binarize_success(PrintHost::PrintHostConfig config, PrintHost::PrintHostJobData data)
{
    process_gcode_inner(std::move(config), std::move(data));
}

void ResultExportInteractor::on_result_export_binarize_fail(const std::string& msg)
{
    SPDLOG_ERROR("ResultExportDataFinalizer has failed: {}", msg);
}

} // namespace Slic3r::Biz::PrintHost
