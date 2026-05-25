#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterInteractor.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountInteractor.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"

#include "Slic3r/Log.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <variant>

namespace Slic3r::Biz::PhysicalPrinter {

namespace {
PhysicalPrinterConfig settings_to_printer(const Domain::PhysicalPrinterSettings& settings, const Domain::Preset::HwPrinterConfig& hw_config)
{
    auto get_str = [&settings](const char* key) {
        return settings.items.opt(key).get<std::string>();
    };

    PrinterUpload auth{
        .type                   = settings.items.opt("physical_printer_host_type").get<Domain::PrintHostType>(),
        .ca_file                = get_str("physical_printer_ca_file"),
        .port                   = get_str("physical_printer_port"),
        .auth_type              = settings.items.opt("physical_printer_authorization_type").get<Domain::PrintHostAuthType>(),
        .ssl_revoke_best_effort = settings.items.opt("physical_printer_ssl_ignore_revoke").get<bool>()
    };

    if (auth.auth_type == Domain::PrintHostAuthType::ApiKey) {
        auth.api_key = get_str("physical_printer_api_key");
    } else if (auth.auth_type == Domain::PrintHostAuthType::Digest) {
        auth.username = get_str("physical_printer_user");
        auth.password = get_str("physical_printer_password");
    }

    return PhysicalPrinterConfig{
        .payload    = std::move(auth),
        .host       = get_str("physical_printer_host"),
        .name       = get_str("physical_printer_user_given_name"),
        .uuid       = get_str("physical_printer_uuid"),
        .hw_config  = hw_config
    };
}
} // namespace

PhysicalPrinterInteractor::PhysicalPrinterInteractor(
    Platform::IMainThreadDispatcher& dispatcher,
    Preset::PresetInteractor& preset_interactor,
    UserAccount::UserAccountInteractor& user_account_interactor
) :
    m_dispatcher(dispatcher),
    m_preset_interactor(preset_interactor),
    m_user_account_interactor(user_account_interactor),
    m_cbi(m_cbi_accessor, &m_storage.dummy_settings())
{
    read_storage();
    m_selected_uuid = m_observable_list.at(0).uuid;
    m_selected_index = 0;
}

PhysicalPrinterInteractor::~PhysicalPrinterInteractor()
{
    ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
}

void PhysicalPrinterInteractor::read_storage()
{
    std::vector<PhysicalPrinterConfig> printers;
    printers.reserve(m_storage.all_settings().size() + 3);
    
    printers.push_back(PhysicalPrinter::filesystem_export_local());
    printers.push_back(PhysicalPrinter::filesystem_export_removable());
    printers.push_back(PhysicalPrinter::connect_upload_generic());

    const UuidSettingsMap& settings_map = m_storage.all_settings();
    for (const auto& [uuid, settings] : settings_map) {
        PhysicalPrinterConfig printer = settings_to_printer(settings, m_storage.hw_config(uuid));

        auto it = std::find_if(
            printers.begin(),
            printers.end(),
            [&printer](const auto& p) {
                return std::holds_alternative<PrinterUpload>(printer.payload) && 
                       std::holds_alternative<PrinterUpload>(p.payload) && 
                       printer.uuid == p.uuid; 
            }
        );

        if (it != printers.end()) {
            *it = std::move(printer);
        } else {
            printers.push_back(std::move(printer));
        }
    }

    m_observable_list.reset(std::move(printers));
}

void PhysicalPrinterInteractor::add_printer_settings(
    Domain::PhysicalPrinterSettings&& settings,
    const std::string& filename
)
{
    std::string uuid{boost::uuids::to_string(boost::uuids::random_generator()())};
    m_observable_list.append(settings_to_printer(settings, m_storage.hw_config(uuid)));
    m_storage.add_printer_settings(std::move(settings), filename);
}

bool PhysicalPrinterInteractor::can_be_selected(const std::string& uuid) const
{
    auto index = index_of(uuid);
    const auto& printer = m_observable_list.at(index);
    if (std::holds_alternative<ConnectUpload>(printer.payload)) {
        return m_user_account_interactor.is_logged_in();
    }
    return true;
}

void PhysicalPrinterInteractor::select_uuid(const std::string& uuid)
{
    m_selected_uuid = uuid;
    m_selected_index = index_of(uuid);
    m_container_to_printer_uuid_map[m_current_container] = m_selected_uuid;
    const auto& current_printer = m_observable_list.at(m_selected_index);

    if (std::holds_alternative<PrinterUpload>(current_printer.payload)) {
        m_cbi_accessor.set_config_box(&m_storage.all_settings().at(current_printer.uuid));
    } else {
        m_cbi_accessor.set_config_box(&m_storage.dummy_settings());
    }

    this->invoke_listeners<IPhysicalPrinterChangedListener>(
        [](auto* listener) { 
            listener->on_selected_physical_printer_changed(); 
        }
    );
}

void PhysicalPrinterInteractor::select_default()
{
    if (m_observable_list.size() == 0) {
        return;
    }
    select_uuid(m_observable_list.at(0).uuid);
}

void PhysicalPrinterInteractor::select_connect_upload(bool prefer_physical_printer)
{
    if (m_selected_index != 0 && prefer_physical_printer) {
        return;
    }

    // Note: This implementation works only in case there is single connect upload item
    for (size_t i = 0; i < m_observable_list.size(); ++i) {
        if (std::holds_alternative<ConnectUpload>(m_observable_list.at(i).payload)) {
            select_uuid(m_observable_list.at(i).uuid);
            return;
        }
    }
    DEBUG_ASSERT(false, "ConnectUpload missing in list of physical printers");
}

void PhysicalPrinterInteractor::remove_uuid(const std::string& uuid)
{
    size_t index = index_of(uuid);
    ASSERT(index != 0);
    bool was_selected = (uuid == m_selected_uuid);

    m_storage.remove_one(uuid);
    m_observable_list.remove({index, index}); 
    
    // Always call some select method after removal to invoke listeners
    if (was_selected) {
        select_default();
    } else {
        select_uuid(m_selected_uuid);
    }
}

ConfigBoxInteractor* PhysicalPrinterInteractor::cbi()
{
    return &m_cbi;
}

void PhysicalPrinterInteractor::save_new_printer()
{
    ASSERT(m_selected_index == 0);
    const Domain::Preset::HwPrinterConfig& current_printer_config = m_preset_interactor.current_printer_config();
    
    std::string uuid = m_storage.create_from_dummy(current_printer_config);
    PhysicalPrinterConfig printer = settings_to_printer(m_storage.all_settings().at(uuid), current_printer_config);
    
    m_observable_list.append(std::move(printer));
    select_uuid(uuid); // Reuses the internal logic directly via UUID
}   

void PhysicalPrinterInteractor::on_dialog_button_add_new()
{
    select_default();
}

bool PhysicalPrinterInteractor::is_filesystem_export_selected() const
{
    return std::holds_alternative<FileSystemExport>(m_observable_list.at(m_selected_index).payload);
}

bool PhysicalPrinterInteractor::is_printer_upload_selected() const
{
    return std::holds_alternative<PrinterUpload>(m_observable_list.at(m_selected_index).payload);
}

bool PhysicalPrinterInteractor::is_connect_upload_selected() const
{
    return std::holds_alternative<ConnectUpload>(m_observable_list.at(m_selected_index).payload);
}

const Domain::ConfigValue* PhysicalPrinterInteractor::get_override_original_value(const Domain::ConfigItem& item, size_t index) const
{
    return m_cbi.find(item.name());
}

void PhysicalPrinterInteractor::set_item_value(const Domain::ConfigItem& item, const Domain::ConfigValue& value, size_t index)
{
    m_cbi_accessor.set_value(item.name(), value);
    
    size_t current_idx = m_selected_index;
    if (current_idx != 0) {
        // Edited printer is existing configuration. Save after each changed value.
        const auto& hw_config = m_storage.hw_config(m_selected_uuid);
        m_storage.save_one(m_selected_uuid, hw_config);
        
        m_observable_list.set(
            settings_to_printer(m_storage.all_settings().at(m_selected_uuid), hw_config), 
            current_idx
        );
        
        invoke_listeners<IPhysicalPrinterChangedListener>([](auto* l) { l->on_printer_data_changed(); });
    }
}

void PhysicalPrinterInteractor::set_item_override(const Domain::ConfigItem& item, bool enable, size_t index) 
{
}

ObservableList<PhysicalPrinterConfig>&  PhysicalPrinterInteractor::observable_list()
{
    return m_observable_list;
}

const ObservableList<PhysicalPrinterConfig>&  PhysicalPrinterInteractor::observable_list() const
{
    return m_observable_list;
}

size_t PhysicalPrinterInteractor::index_of(const std::string& uuid) const
{
    for (size_t i = 0; i < m_observable_list.size(); ++i) {
        if (m_observable_list.at(i).uuid == uuid) {
            return i;
        }
    }
    ASSERT(false);
    return 0;
}

const PhysicalPrinterConfig& PhysicalPrinterInteractor::selected_physical_printer_data()
{
    return m_observable_list.at(m_selected_index);
}

bool PhysicalPrinterInteractor::is_printer_compatible(const std::string& uuid, const Domain::Preset::HwPrinterConfig& config) const
{
    if (uuid == m_observable_list.at(0).uuid) {  // Default is always compatible
        return true;
    }
    
    const auto& printer = m_observable_list.at(index_of(uuid));

    if (std::holds_alternative<ConnectUpload>(printer.payload)) {
        return true;
    }

    if (std::holds_alternative<PrinterUpload>(printer.payload)) {
        return is_physical_printer_compatible(printer, config);
    }
    
    return false;
}

void PhysicalPrinterInteractor::on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id)
{
    m_current_container = ContainerKey{project_id, container_id};
    
    bool hw_compatible = is_printer_compatible(m_selected_uuid, m_preset_interactor.current_printer_config());
    if (auto it = m_container_to_printer_uuid_map.find(m_current_container); it != m_container_to_printer_uuid_map.end()) {
        if (can_be_selected(it->second)) {
            select_uuid(it->second);
            return;
        }
    } else if (can_be_selected(m_selected_uuid) && hw_compatible) {
        m_container_to_printer_uuid_map.emplace(m_current_container, m_selected_uuid);
        return;
    }

    select_default();
}

} // namespace Slic3r::Biz::PhysicalPrinter