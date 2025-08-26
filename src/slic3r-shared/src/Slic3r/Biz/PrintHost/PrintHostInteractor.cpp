#include "Slic3r/Biz/PrintHost/PrintHostInteractor.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"

#include "libslic3r/format.hpp"

#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

PrintHostInteractor::PrintHostInteractor(Platform::IMainThreadDispatcher& dispatcher) :
    m_print_host_data_finalizer(dispatcher)
{
    m_print_host_data_finalizer.add_listener<PrintHost::IPrintHostBinarizeListener>(this);
}

void PrintHostInteractor::on_storage_resolved(size_t id, const std::string& storage)
{
    if (auto node = m_storage_callbacks_map.extract(id); !node.empty()) {
        node.mapped()(storage);
    }
}

void PrintHostInteractor::export_gcode(PrintHostConfig config, PrintHostJobData data)
{
    m_print_host_data_finalizer.finalize(std::move(config), std::move(data));
}

void PrintHostInteractor::upload_gcode(PrintHostConfig config, PrintHostJobData data)
{
    m_print_host_data_finalizer.finalize(std::move(config), std::move(data));
}

void PrintHostInteractor::process_gcode_inner(PrintHostConfig config, PrintHostJobData data)
{
    if (config.type == Domain::PrintHostType::PrusaLink) {
        upload_gcode_with_storage_choice(std::move(config), std::move(data));
        return;
    }
    size_t id = m_print_host_job_manager.emplace_job(std::move(config), std::move(data));
}

void PrintHostInteractor::upload_gcode_with_storage_choice(PrintHostConfig config, PrintHostJobData data)
{
    // Since copy constructor and assign are deleted for PrintHostConfig and PrintHostJobData,
    // We use shared pointer to be able to move them to the callback lambda and copy the lambda
    auto config_ptr = std::make_shared<PrintHostConfig>(std::move(config));
    auto data_ptr   = std::make_shared<PrintHostJobData>(std::move(data));

    StorageInfoFn callback = [this, config_ptr, data_ptr](const std::string& storage) mutable
    {
        data_ptr->storage = storage;
        m_print_host_job_manager.emplace_job(std::move(*config_ptr), std::move(*data_ptr));
    };

    // Create PrusaLinkStorage config by copying the shared pointer.
    // PrintHostConfig has deleted copy constructor.
    PrintHostConfig storage_config = {Domain::PrintHostType::PrusaLinkStorage, config_ptr->host};
    storage_config.ca_file         = config_ptr->ca_file;
    storage_config.ssl_revoke_best_effort = config_ptr->ssl_revoke_best_effort;
    storage_config.port                   = config_ptr->port;
    storage_config.auth_type              = config_ptr->auth_type;
    storage_config.api_key                = config_ptr->api_key;
    storage_config.username               = config_ptr->username;
    storage_config.password               = config_ptr->password;

    size_t id = m_print_host_job_manager.emplace_job(std::move(storage_config), {nullptr, data_ptr->dest_path, PrintHostExportFormat::Undefined});

    m_storage_callbacks_map[id] = std::move(callback);
}

void PrintHostInteractor::on_print_host_binarize_success(PrintHostConfig config, PrintHostJobData data)
{
    process_gcode_inner(std::move(config), std::move(data));
}

void PrintHostInteractor::on_print_host_binarize_fail(const std::string& msg)
{
    SPDLOG_ERROR("PrintHostDataFinalizer has failed: {}", msg);
}

} // namespace Slic3r::Biz::PrintHost
