#pragma once

#include "Slic3r/App/ConfigSettingsDialog.hpp"
#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::PhysicalPrinter {
class PhysicalPrinterInteractor;
} // namespace Slic3r::Biz::PhysicalPrinter


namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;
class PhysicalPrinterSettingsDialog;

class PhysicalPrinterAdvancedSettingsDialog :
    public ConfigSettingsDialog,
    public Biz::PhysicalPrinter::IPhysicalPrinterChangedListener
{
public:
    explicit PhysicalPrinterAdvancedSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        PhysicalPrinterSettingsDialog* physical_printer_settings_dialog
    );
    ~PhysicalPrinterAdvancedSettingsDialog() = default;

    void on_selected_physical_printer_changed() override;

protected:
    void close_action() override;

private:
    Biz::ListenerScope<
        Biz::PhysicalPrinter::IPhysicalPrinterChangedListener,
        Biz::PhysicalPrinter::PhysicalPrinterInteractor,
        PhysicalPrinterAdvancedSettingsDialog>
        m_physical_printer_changed_listener_scope;

    PhysicalPrinterSettingsDialog* m_physical_printer_settings_dialog{nullptr}; 
    Yoga::LayoutButton* m_save_button{nullptr};
};
} // namespace Slic3r::App