#include "Slic3r/Biz/PrintHost/PrintHostInteractor.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"

#include "libslic3r/format.hpp"

#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

namespace {
void get_storage_choices_from_json(const std::string& json_text, std::vector<PrintHostStorageInfo>& storage_list)
{
    /*
    [{
        "name": "Storage",
        "path": "/mnt/storage",
        "read_only": false,
        "free_space": 123456789
    },
    {
        "name": "usb",
        "path": "/usb",
        "read_only": false
        "free_space": 123456789
    }]
    */

    try {
        nlohmann::json json = nlohmann::json::parse(json_text);

        for (const auto& item  : json) {
            PrintHostStorageInfo storage;
            storage.name = item.value("name", "");
            storage.path = item.value("path", "");
            storage.read_only = item.value("read_only", false);
            storage.free_space = item.value("free_space", -1);
            
            storage_list.emplace_back(std::move(storage));
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR(format("Error parsing JSON: %1%", e.what()));
    }
}
} // namespace

PrintHostInteractor::PrintHostInteractor(Platform::IMainThreadDispatcher& dispatcher)
    : m_print_host_job_manager(dispatcher)
{ 
    m_print_host_job_manager.add_listener<PrintHost::IPrintHostListener>(this);
}

void PrintHostInteractor::on_print_host_progress(size_t id, int progress) 
{ 
    SPDLOG_INFO("ProjectInteractor::on_print_host_progress id:{} progress: {}", std::to_string(id), std::to_string(progress));
}
void PrintHostInteractor::on_print_host_error(size_t id, const std::string& msg) 
{ 
    SPDLOG_ERROR("ProjectInteractor::on_print_host_error id:{} msg: {}", std::to_string(id), msg); 
}
void PrintHostInteractor::on_print_host_cancel(size_t id) 
{ 
    SPDLOG_INFO("ProjectInteractor::on_print_host_cancel id:{}", std::to_string(id)); 
}
void PrintHostInteractor::on_print_host_done(size_t id) 
{
    SPDLOG_INFO("ProjectInteractor::on_print_host_done id:{}", std::to_string(id)); 
}
void PrintHostInteractor::on_print_host_info(size_t id, const std::string& tag, const std::string& msg) 
{
    SPDLOG_INFO("ProjectInteractor::on_print_host_info id:{} tag: {} msg: {}", std::to_string(id), tag, msg); 

    if (tag == "storage" && m_storage_callbacks_map.find(id) != m_storage_callbacks_map.end()) {
        m_storage_callbacks_map[id](msg);
        m_storage_callbacks_map.erase(id);
    }
}

void PrintHostInteractor::export_gcode(PrintHostConfig config, PrintHostJobData data)
{
    SPDLOG_INFO("Export gcode to {}", data.dest_path.string());
    size_t id = m_print_host_job_manager.emplace_job(std::move(config), std::move(data));
}


void PrintHostInteractor::upload_gcode(PrintHostConfig config, PrintHostJobData data)
{
    if (config.type == PrintHostType::PrusaLink) {
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
    auto data_ptr = std::make_shared<PrintHostJobData>(std::move(data));

    StorageInfoFn callback = [this, config_ptr, data_ptr](const std::string& json) mutable {
        std::vector<PrintHostStorageInfo> storage_list;
        get_storage_choices_from_json(json, storage_list);
        // TODO: here user should choose storage
        // for now we just use the first one
        if (storage_list.empty()) {
            return;
        }
        data_ptr->storage = storage_list[0].path;
        m_print_host_job_manager.emplace_job(std::move(*config_ptr), std::move(*data_ptr));
    };

    // Create PrusaLinkStorage config by copying the shared pointer.
    // PrintHostConfig has deleted copy constructor.
    PrintHostConfig storage_config = {Slic3r::Biz::PrintHost::PrintHostType::PrusaLinkStorage
        , config_ptr->host
        , config_ptr->ca_file
        , config_ptr->ssl_revoke_best_effort
        , config_ptr->port
    };
    storage_config.auth_type = config_ptr->auth_type;
    storage_config.api_key = config_ptr->api_key;
    storage_config.username = config_ptr->username;
    storage_config.password = config_ptr->password;

    size_t id = m_print_host_job_manager.emplace_job(
        std::move(storage_config),
        {std::string(), data_ptr->dest_path});

    m_storage_callbacks_map[id] = std::move(callback);
}

} // namespace Slic3r::Biz::PrintHost