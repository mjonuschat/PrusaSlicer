#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/MaterialState.hpp"

namespace Slic3r::App {

namespace Yoga {
class Text;
class PrinterSettingsButton;
class MaterialSettingsButton;
} // namespace Yoga

class SidebarBed : public Yoga::Window
{
public:
    explicit SidebarBed();

private:
    Yoga::Text* m_bed_name{nullptr};
    Yoga::PrinterSettingsButton* m_printer{nullptr};

    Biz::ObservableList<MaterialState> m_observable_list; ///< this will be moved to more appropriate place

    using MaterialListView = Yoga::ListView<Yoga::MaterialSettingsButton, MaterialState>;
    MaterialListView* m_list_view{nullptr};
};

} // namespace Slic3r::App
