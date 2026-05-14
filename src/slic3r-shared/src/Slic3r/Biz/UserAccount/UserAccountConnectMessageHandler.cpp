#include "Slic3r/Biz/UserAccount/UserAccountConnectMessageHandler.hpp"

#include "Slic3r/Biz/UserAccount/UserAccountCommunication.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "Slic3r/Log.hpp"
#include "nlohmann/json.hpp"

namespace Slic3r::Biz::UserAccount {

UserAccountConnectMessageHandler::UserAccountConnectMessageHandler(
    Platform::IMainThreadDispatcher& dispatcher
) :
    m_dispatcher(dispatcher)
{}

void UserAccountConnectMessageHandler::handle_select_printer_message(
    UserAccountCommunication& communication,
    const std::string& message_json
)
{
    std::string uuid;
    try {
        nlohmann::json j = nlohmann::json::parse(message_json);
        j.at("uuid").get_to(uuid);
    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("JSON parsing error: {}", e.what());
    }
    if (uuid.empty()) {
        SPDLOG_ERROR("Failed to select printer from Connect: Failed to parse Connect message.");
        return;
    }

    auto succ_fn = [this, uuid](const std::string& body)
    { parse_connect_printers_for_selection(body, uuid); };

    communication.enqueue_connect_printers_data_action(std::move(succ_fn));
}

void UserAccountConnectMessageHandler::parse_connect_printers_for_selection(
    const std::string& message_json,
    const std::string& uuid
)
{
    try {
        nlohmann::json j = nlohmann::json::parse(message_json);

        if (!j.contains("printers") || !j["printers"].is_array()) {
            SPDLOG_ERROR("Connect printers JSON is missing the 'printers' array.");
            return;
        }

        for (const auto& printer : j["printers"]) {
            if (printer.contains("uuid") && printer["uuid"] == uuid) {
                m_dispatcher.dispatch_on_main_thread(
                    [this, printer]()
                    {
                        this->invoke_listeners<IUserAccountListener>(
                            [printer](auto* listener)
                            { listener->on_select_printer_from_connect(printer.dump()); }
                        );
                    }
                );

                return;
            }
        }

        SPDLOG_WARN("Printer with UUID {} not found in Connect message.", uuid);

    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Failed to parse Connect printers JSON: {}", e.what());
    }
}

namespace {

bool
compare_feature(const Domain::Preset::FeatureValue& hw_feature, const nlohmann::json& json_feature)
{
    return std::visit(
        [&json_feature](const auto& val) -> bool
        {
            using T = std::decay_t<decltype(val)>;

            if constexpr (std::is_same_v<T, double>) {
                if (!json_feature.is_number()) {
                    return false;
                }
                constexpr double epsilon = 1e-6; 
                return std::abs(json_feature.get<double>() - val) <= epsilon;
            } else if constexpr (std::is_same_v<T, bool>) {
                return json_feature.is_boolean() && json_feature.get<bool>() == val;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return json_feature.is_string() && json_feature.get<std::string>() == val;
            } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return json_feature.is_null();
            } else if constexpr (std::is_same_v<T, Domain::JsonArray>) {
                if (!json_feature.is_array() || json_feature.size() != val.size()) {
                    return false;
                }
                for (size_t i = 0; i < val.size(); ++i) {
                    if (!compare_feature(val[i], json_feature[i])) {
                        return false;
                    }
                }
                return true;
            } else if constexpr (std::is_same_v<T, Domain::JsonObject>) {
                if (!json_feature.is_object() || json_feature.size() != val.size()) {
                    return false;
                }
                for (const auto& [key, inner_val] : val) {
                    if (!json_feature.contains(key)
                        || !compare_feature(inner_val, json_feature[key])) {
                        return false;
                    }
                }
                return true;
            }

            return false;
        },
        static_cast<const Domain::JsonVariant&>(hw_feature)
    );
}

const Domain::Preset::HwPrinterConfig*
find_matching_printer_config(const auto& printer_configs_view, const nlohmann::json& j)
{
    std::string j_model        = j.value("model", "");
    std::string j_base_model   = j.value("base_model", "");
    uint8_t j_tool_count       = j.value("tool_count", static_cast<uint8_t>(0));
    size_t j_tools_array_size  = j.contains("tools") ? j["tools"].size() : 0;

    for (const auto& item : printer_configs_view) {
        const auto& hw_config = item.first.get();
        if (hw_config.model.model == j_model
            && hw_config.model.base_model == j_base_model
            && hw_config.tool_count == j_tool_count
            && hw_config.material_slot_count() == j_tools_array_size)
        {
            return &hw_config;
        }
    }
    return nullptr;
}

const Preset::PresetItem* find_matching_preset_item(
    const auto& printer_presets,
    const Domain::Preset::HwPrinterConfig* hw_config
)
{
    ASSERT(hw_config);

    for (size_t i = 0; i < printer_presets.size(); i++) {
        const Preset::PresetItem& item = printer_presets.at(i);

        if (item.hw_printer_config_id == hw_config->id
            && item.origin == Domain::Preset::PresetOrigin::System)
        {
            return &item;
        }
    }

    return nullptr;
}

void select_printer_tools_from_connect(auto& preset_interactor, const nlohmann::json& j)
{
    if (!j.contains("tools") || !j["tools"].is_object()) {
        return;
    }

    size_t tool_idx = 0;

    for (const auto& [tool_key, tool_json] : j["tools"].items()) {
        if (!tool_json.contains("features") || tool_idx >= preset_interactor.tool_items().size()) {
            tool_idx++;
            continue;
        }

        const auto& tool_list       = preset_interactor.tool_items().at(tool_idx);
        const auto& available_tools = tool_list.items();

        std::string matching_tool_id;

        for (size_t i = 0; i < available_tools.size(); ++i) {
            const Domain::Preset::HwToolConfigDef& tool_def = available_tools.at(i);
            bool matches                                    = true;

            for (const auto& [feature_key, json_feature_val] : tool_json["features"].items()) {
                auto it = tool_def.features.find(feature_key);

                if (it == tool_def.features.end()) {
                    continue;
                }

                if (!compare_feature(it->second.default_value, json_feature_val)) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                matching_tool_id = tool_def.id;
                break;
            }
        }

        if (!matching_tool_id.empty()) {
            preset_interactor.select_printer_tool_item(tool_idx, matching_tool_id);
        } else {
            SPDLOG_WARN(
                "Connect tool sync: No matching tool definition found for tool index {}",
                tool_idx
            );
        }

        tool_idx++;
    }
}

void select_printer_materials_from_connect(auto& preset_interactor, const nlohmann::json& j)
{
    if (!j.contains("tools") || !j["tools"].is_object()) {
        return;
    }

    const auto& tools_json = j["tools"];
    size_t max_slots = preset_interactor.material_presets().size();

    for (size_t slot_idx = 0; slot_idx < max_slots; ++slot_idx) {
        std::string tool_key = std::to_string(slot_idx);
        
        if (!tools_json.contains(tool_key)) {
            continue;
        }

        const auto& tool_json = tools_json[tool_key];
        std::string target_type;

        if (tool_json.contains("material_package_instance") && tool_json["material_package_instance"].is_object()) {
            const auto& mpi = tool_json["material_package_instance"];
            
            if (mpi.contains("package") && mpi["package"].is_object()) {
                const auto& pkg = mpi["package"];
                
                if (pkg.contains("material") && pkg["material"].is_object()) {
                    const auto& mat = pkg["material"];
                    
                    if (mat.contains("type") && mat["type"].is_string()) {
                        target_type = mat["type"].get<std::string>();
                    }
                }
            }
        }

        if (target_type.empty()) {
            continue;
        }

        const auto& material_list       = preset_interactor.material_presets().at(slot_idx);
        const auto& available_materials = material_list.items();
        std::string matching_material_id;

        for (size_t i = 0; i < available_materials.size(); ++i) {
            const Preset::PresetItem& preset = available_materials.at(i);
            
            if (preset.name.find(target_type) != std::string::npos || 
                preset.id.find(target_type) != std::string::npos) 
            {
                matching_material_id = preset.id;
                break;
            }
        }

        if (!matching_material_id.empty()) {
            preset_interactor.select_material_preset(slot_idx, matching_material_id);
        } else {
            SPDLOG_WARN("Connect material sync: No matching material preset found for slot {} (Target type: {})", slot_idx, target_type);
        }
    }
}

} // namespace

void UserAccountConnectMessageHandler::do_select_printer_from_connect(
    ProjectInteractor& project_interactor,
    const std::string& printer_json
)
{
    nlohmann::json parsed_printer_json;
    try {
        parsed_printer_json = nlohmann::json::parse(printer_json);
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("JSON parsing error: {}", e.what());
        return;
    }

    auto& preset_interactor          = project_interactor.preset_interactor();
    const auto& printer_configs_view = preset_interactor.get_printer_configs();
    
    const Domain::Preset::HwPrinterConfig* config =
        find_matching_printer_config(printer_configs_view, parsed_printer_json);

    if (!config) {
        SPDLOG_INFO("Failed to select printer preset from Connect. No matching hw config.");
        return;
    }

    const Preset::PresetItem* item =
        find_matching_preset_item(preset_interactor.printer_presets().items(), config);

    if (!item) {
        SPDLOG_INFO("Failed to select printer preset from Connect. No matching printer preset.");
        return;
    }

    preset_interactor.select_printer_preset(config->id, item->id);
    select_printer_tools_from_connect(preset_interactor, parsed_printer_json);
    select_printer_materials_from_connect(preset_interactor, parsed_printer_json);
    project_interactor.physical_printer_interactor().select_connect_upload(false);

    m_last_printer_json = printer_json;
}

std::string UserAccountConnectMessageHandler::uuid_for_upload(const ProjectInteractor& project_interactor)
{
    if (m_last_printer_json.empty()) {
        return {};
    }

    try {
        nlohmann::json j = nlohmann::json::parse(m_last_printer_json);
        const auto& current_config = project_interactor.preset_interactor().current_printer_config();

        std::string j_model        = j.value("model", "");
        std::string j_base_model   = j.value("base_model", "");
        uint8_t j_tool_count       = j.value("tool_count", static_cast<uint8_t>(0));
        size_t j_tools_array_size  = j.contains("tools") ? j["tools"].size() : 0;

        if (current_config.model.model == j_model
            && current_config.model.base_model == j_base_model
            && current_config.tool_count == j_tool_count
            && current_config.material_slot_count() == j_tools_array_size)
        {
            return j.value("uuid", "");
        }
    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Failed to parse printer json: {}", e.what());
    }

    return {};
}

} // namespace Slic3r::Biz::UserAccount
