#pragma once

#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterConfig.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterStorage.hpp"
#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

#include <vector>
#include <string>

namespace Slic3r::Domain::Preset {
struct HwPrinterConfig;
}

namespace Slic3r::Biz::Preset {
class PresetInteractor;
}

namespace Slic3r::Biz::PhysicalPrinter {

class PhysicalPrinterInteractor : 
    public WithListeners<IPhysicalPrinterChangedListener>,
    public Biz::IConfigBoxSetter
{
public:
    PhysicalPrinterInteractor(Platform::IMainThreadDispatcher& dispatcher, Preset::PresetInteractor& preset_interactor);
    ~PhysicalPrinterInteractor();

    ObservableList<PhysicalPrinterConfig>&  observable_list();

    const ObservableList<PhysicalPrinterConfig>&  observable_list() const;

    void select_uuid(const std::string& uuid);
    void select_default();
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

    bool is_printer_compatible(const std::string& uuid, const Domain::Preset::HwPrinterConfig& config);

private:
    void read_storage();
     
    size_t index_of(const std::string& uuid) const;

    void add_printer_settings(Domain::PhysicalPrinterSettings&& settings, const std::string& filename);

private:
    Platform::IMainThreadDispatcher& m_dispatcher;
    Preset::PresetInteractor& m_preset_interactor;
    PhysicalPrinterStorage m_storage;
    ObservableList<PhysicalPrinterConfig> m_observable_list;

    ConfigBoxInteractor::SetAccessor m_cbi_accessor;
    ConfigBoxInteractor m_cbi;

    std::string m_selected_uuid;
    size_t      m_selected_index;
};
} //namespace Slic3r::Biz::PhysicalPrinter