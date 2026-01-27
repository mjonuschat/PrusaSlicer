#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"

#include "Slic3r/Log.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Slic3r::Biz::PhysicalPrinter {

namespace {
PhysicalPrinterConfig settings_to_printer(const Domain::PhysicalPrinterSettings& settings)
{
    PhysicalPrinterConfig printer;
    printer.host = settings.items.opt("physical_printer_host").get<std::string>();
    printer.name = settings.items.opt("physical_printer_user_given_name").get<std::string>();
    printer.uuid = settings.items.opt("physical_printer_uuid").get<std::string>();
    printer.base_model = settings.items.opt("physical_printer_preset_base_model").get<std::string>();

    printer.operation_type = OperationType::PrintHost;
    LocalAuth auth;
    auth.type = settings.items.opt("physical_printer_host_type").get<Domain::PrintHostType>();

    auth.auth_type =
        settings.items.opt("physical_printer_authorization_type").get<Domain::PrintHostAuthType>();
    if (auth.auth_type == Domain::PrintHostAuthType::ApiKey) {
        auth.api_key = settings.items.opt("physical_printer_api_key").get<std::string>();
    } else if (auth.auth_type == Domain::PrintHostAuthType::Digest) {
        auth.password = settings.items.opt("physical_printer_password").get<std::string>();
        auth.username = settings.items.opt("physical_printer_user").get<std::string>();
    }

    auth.ca_file                = settings.items.opt("physical_printer_ca_file").get<std::string>();
    auth.port                   = settings.items.opt("physical_printer_port").get<std::string>();
    auth.ssl_revoke_best_effort = settings.items.opt("physical_printer_ssl_ignore_revoke").get<bool>();

    printer.connection_data = std::move(auth);

    return printer;
}
} // namespace

PhysicalPrinterInteractor::PhysicalPrinterInteractor(Platform::IMainThreadDispatcher& dispatcher, Preset::PresetInteractor& preset_interactor) :
    m_dispatcher(dispatcher),
    m_preset_interactor(preset_interactor),
    m_cbi(m_cbi_accessor, &m_storage.dummy_settings())
{
    read_storage();
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
    printers.push_back(PhysicalPrinter::none());

    const UuidSettingsMap& settings_map = m_storage.all_settings();
    for (const auto& [uuid, settings] : settings_map) {
        PhysicalPrinterConfig printer = settings_to_printer(settings);

        auto it = std::find_if(
            printers.begin(),
            printers.end(),
            [&printer](const auto& p) {
                const auto* new_local = std::get_if<LocalAuth>(&printer.connection_data);
                const auto* existing_local = std::get_if<LocalAuth>(&p.connection_data);
                if (new_local && existing_local) {
                    return printer.uuid == p.uuid;
                }
                return false; 
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
    m_observable_list.append(settings_to_printer(settings));
    m_storage.add_printer_settings(std::move(settings), filename);
}

void PhysicalPrinterInteractor::select_index(size_t index)
{
    ASSERT(index < m_observable_list.size());
    m_selected_index = index;

    const auto* data = std::get_if<LocalAuth>(&m_observable_list.at(m_selected_index).connection_data);
    if (data) {
        auto* configbox = &m_storage.all_settings().at(m_observable_list.at(m_selected_index).uuid);
        m_cbi_accessor.set_config_box(configbox);
    } else {
        auto* configbox = &m_storage.dummy_settings();
        m_cbi_accessor.set_config_box(configbox);
    }

    this->invoke_listeners<IPhysicalPrinterChangedListener>(
        [this](auto* listener) { 
            listener->on_selected_physical_printer_changed(); 
        }
    );
}

void PhysicalPrinterInteractor::remove_selected()
{
    ASSERT(m_selected_index != 0);
    const size_t index = m_selected_index;
    const std::string uuid =m_observable_list.at(m_selected_index).uuid;
    select_index(0);
    m_storage.remove_one(uuid);
    m_observable_list.remove(index);
}

ConfigBoxInteractor* PhysicalPrinterInteractor::cbi()
{
    return &m_cbi;
}

void PhysicalPrinterInteractor::save_current_edit()
{
    ASSERT(m_selected_index == 0);
    ASSERT(m_observable_list.at(0).operation_type == OperationType::None);
    const Domain::Preset::HwPrinterConfig& current_printer_config = m_preset_interactor.current_printer_config();
    const std::string model = current_printer_config.model.model;
    const std::string base_model = current_printer_config.model.base_model;
    std::string uuid = m_storage.store_dummy(model, base_model);
    PhysicalPrinterConfig printer = settings_to_printer(m_storage.all_settings().at(uuid));
    m_observable_list.append(std::move(printer));
    select_index(m_observable_list.size()-1);
}   

void PhysicalPrinterInteractor::on_dialog_button_add_new()
{
    select_index(0);
}

bool PhysicalPrinterInteractor::is_none_selected()
{
    return m_observable_list.at(m_selected_index).operation_type == OperationType::None;
}

bool PhysicalPrinterInteractor::is_local_auth_selected()
{
    return  m_observable_list.at(m_selected_index).operation_type == OperationType::PrintHost;
}

const Domain::ConfigValue* PhysicalPrinterInteractor::get_override_original_value(const Domain::ConfigItem& item, size_t index) const
{
    return m_cbi.find(item.name());
}

void PhysicalPrinterInteractor::set_item_value(const Domain::ConfigItem& item, const Domain::ConfigValue& value, size_t index)
{
    m_cbi_accessor.set_value(item.name(), value);
    
    if (m_selected_index == 0) {
        // Edited printer is a new edit - we do not save until saved by user.
    } else {
        // Edited printer is existing configuration. Save after each changed value.
        const std::string edited_uuid =m_observable_list.at(m_selected_index).uuid;
        m_storage.save_one(edited_uuid);
        // also update m_printers
        m_observable_list.set(settings_to_printer(m_storage.all_settings().at(edited_uuid)), m_selected_index);
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

size_t PhysicalPrinterInteractor::selected_index() 
{
    return m_selected_index;
}

const PhysicalPrinterConfig& PhysicalPrinterInteractor::selected_physical_printer_data()
{
    ASSERT(m_selected_index < m_observable_list.size());
    return m_observable_list.at(m_selected_index);
}

bool PhysicalPrinterInteractor::is_printer_on_index_compatible(size_t index, const Domain::Preset::HwPrinterConfig& config)
{
    if (index == 0) {  // "None" is always compatible
        return true;
    }
    const std::string model = config.model.model;
    const std::string base_model = config.model.base_model;

    const auto* data = std::get_if<LocalAuth>(&m_observable_list.at(index).connection_data);
    if (data) {
        auto* configbox = &m_storage.all_settings().at(m_observable_list.at(index).uuid);
        return configbox->find("physical_printer_preset_base_model").item->get<std::string>() == base_model;
    }
    return false;
}

} // namespace Slic3r::Biz::PhysicalPrinter
