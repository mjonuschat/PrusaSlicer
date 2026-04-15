#pragma once

#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

#include "Slic3r/App/ConfigSettingsDialog.hpp"

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

class PhysicalPrinterAdvancedSettingsDialog :
    public ConfigSettingsDialog,
    public Biz::PhysicalPrinter::IPhysicalPrinterChangedListener
{
public:
    explicit PhysicalPrinterAdvancedSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator
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

    Yoga::LayoutButton* m_save_button{nullptr};
};
} // namespace Slic3r::App