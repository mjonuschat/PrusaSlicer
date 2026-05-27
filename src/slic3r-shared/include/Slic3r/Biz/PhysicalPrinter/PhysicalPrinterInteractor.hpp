#pragma once

#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterConfig.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterStorage.hpp"
#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

#include <vector>
#include <string>

namespace Slic3r::Domain::Preset {
struct HwPrinterConfig;
}

namespace Slic3r::Biz::Preset {
class PresetInteractor;
}

namespace Slic3r::Biz::UserAccount {
class UserAccountInteractor;
}

namespace Slic3r::Biz::PhysicalPrinter {

class PhysicalPrinterInteractor : 
    public WithListeners<IPhysicalPrinterChangedListener>,
    public Biz::IConfigBoxSetter,
    public ISelectedConfigContainerChangedListener
{
public:
    PhysicalPrinterInteractor(
        Platform::IMainThreadDispatcher& dispatcher,
        Preset::PresetInteractor& preset_interactor,
        UserAccount::UserAccountInteractor& user_account_interactor
    );
    ~PhysicalPrinterInteractor();

    ObservableList<PhysicalPrinterConfig>&  observable_list();

    const ObservableList<PhysicalPrinterConfig>&  observable_list() const;

    bool can_be_selected(const std::string& uuid) const;
    void select_uuid(const std::string& uuid);
    void select_default();
    void select_connect_upload(bool prefer_physical_printer);
    void remove_uuid(const std::string& uuid);
    std::string selected_uuid() const
    {
        return m_selected_uuid;
    }

    bool is_filesystem_export_selected() const;
    bool is_printer_upload_selected() const;
    bool is_connect_upload_selected() const;

    const PhysicalPrinterConfig& selected_physical_printer_data();

    ConfigBoxInteractor* cbi();

    void save_new_printer();

    void on_dialog_button_add_new();

    // IConfigBoxSetter

    const Domain::ConfigValue*
    get_override_original_value(const Domain::ConfigItem& item, size_t index = 0) const override;

   void set_item_value(
        const Domain::ConfigItem& item,
        const Domain::ConfigValue& value,
        size_t index = 0
    ) override;

    void
    set_item_override(const Domain::ConfigItem& item, bool enable, size_t index = 0) override;

    bool is_printer_compatible(const std::string& uuid, const Domain::Preset::HwPrinterConfig& config) const;

    void on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id) override;

private:
    void read_storage();
     
    size_t index_of(const std::string& uuid) const;

    void add_printer_settings(Domain::PhysicalPrinterSettings&& settings, const std::string& filename);

private:
    Platform::IMainThreadDispatcher& m_dispatcher;
    Preset::PresetInteractor& m_preset_interactor;
    UserAccount::UserAccountInteractor& m_user_account_interactor;
    ConfigBoxInteractor::SetAccessor m_cbi_accessor;
    PhysicalPrinterStorage m_storage;
    ConfigBoxInteractor m_cbi;
    ObservableList<PhysicalPrinterConfig> m_observable_list;

    using ContainerKey = std::pair<Domain::SelectionId, Domain::SelectionId>;
    ContainerKey m_current_container {0,0};
    std::map<ContainerKey, std::string> m_container_to_printer_uuid_map;    

    std::string m_selected_uuid;
    size_t      m_selected_index;
};
} //namespace Slic3r::Biz::PhysicalPrinter