#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include <functional>

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App {

namespace Yoga {
class Text;
class Rectangle;
}

class ObjectList;

class ObjectListWindow : public Yoga::Window
{
public:

    ObjectListWindow(Biz::ProjectInteractor* project_interactor, bool for_plater);
    void update_sliced_info();

private:
    Yoga::Text* m_label{ nullptr };
    ObjectList* m_object_list{ nullptr };

    Yoga::Item* m_scene_map{ nullptr };
    Yoga::Rectangle* m_sliced_info{ nullptr };

    Yoga::Text* m_used_material{ nullptr };
    Yoga::Text* m_material_cost{ nullptr };
    Yoga::Text* m_first_layer_time{ nullptr };
    Yoga::Text* m_estimated_time{ nullptr };

    Yoga::Item* m_material_cost_row{ nullptr };
    Yoga::Item* m_first_layer_time_row{ nullptr };

    Biz::ProjectInteractor* m_project_interactor{ nullptr };
};

} // namespace Slic3r::App