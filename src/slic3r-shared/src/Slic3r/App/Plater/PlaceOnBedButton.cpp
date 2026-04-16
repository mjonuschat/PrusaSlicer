#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"

namespace Slic3r::App::Plater {
using Biz::_u8L;

PlaceOnBedButton::PlaceOnBedButton(
    Biz::ProjectInteractor& project_interactor
) :
    Yoga::LayoutButton{_u8L("Place on bed")},
    m_project_interactor(project_interactor),
    m_scene_interactor(project_interactor.scene_interactor())
{
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);
    m_scene_interactor.add_listener<Biz::Scene::ISceneSelectionChangedListener>(this);

    set_flex_grow(1);
    constexpr ImColor color_primary{223, 93, 45};
    set_background_color(color_primary);
    set_label_font_type(Render::ImguiFontType::Bold);
    reload();
}

PlaceOnBedButton::~PlaceOnBedButton()
{
    m_project_interactor.remove_listener<Biz::ISelectedProjectChangedListener>(this);
    m_scene_interactor.remove_listener<Biz::Scene::ISceneSelectionChangedListener>(this);
}

void PlaceOnBedButton::on_scene_selection_bounding_box_updated(
    Domain::SelectionId,
    const Biz::Scene::ObjectSelection&
)
{
    reload();
}

void PlaceOnBedButton::on_selected_project_changed_final(size_t) {
    reload();
}

void PlaceOnBedButton::reload()
{
    const std::optional<Biz::Scene::SelectionExtents> selection_bounding_box{
        m_scene_interactor.selection_bounding_box()
    };
    if (!selection_bounding_box) {
        return;
    }

    if (selection_bounding_box->is_floating()) {
        set_visible(true);
        callbacks().action = [=, this]()
        {
            Domain::SquareMatrix4d relative_transform_world{Domain::SquareMatrix4d::Identity()};
            relative_transform_world.col(3).z() = -selection_bounding_box->min_z();
            m_scene_interactor.transform_selection(relative_transform_world);
        };
        return;
    }

    set_visible(false);
    callbacks().action = []() {};
}

} // namespace Slic3r::App::Plater
