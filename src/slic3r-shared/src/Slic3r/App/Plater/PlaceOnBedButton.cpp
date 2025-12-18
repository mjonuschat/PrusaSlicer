#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"

namespace Slic3r::App::Plater {
using Biz::_u8L;

PlaceOnBedButton::PlaceOnBedButton(
    App::Plater::PlaterScenePresenter& scene_provider,
    Biz::Scene::SceneInteractor& scene_interactor
) :
    Yoga::LayoutButton{_u8L("Place on bed")},
    m_scene_provider(scene_provider),
    m_scene_interactor(scene_interactor)
{
    m_scene_interactor.add_listener<Biz::Scene::ISceneSelectionChangedListener>(this);

    set_flex_grow(1);
    constexpr ImColor color_primary{223, 93, 45};
    set_background_color(color_primary);
    set_label_font_type(Render::ImguiFontType::Bold);
    reload();
}

PlaceOnBedButton::~PlaceOnBedButton()
{
    m_scene_interactor.remove_listener<Biz::Scene::ISceneSelectionChangedListener>(this);
}

void PlaceOnBedButton::on_scene_selection_changed(
    Domain::SelectionId,
    const Biz::Scene::ObjectSelection&
)
{
    reload();
}

void PlaceOnBedButton::on_scene_selection_transformed(
    Domain::SelectionId,
    const Biz::Scene::ObjectSelection&
)
{
    reload();
}

void PlaceOnBedButton::trigger() {
    if (is_floating) {
        callbacks().action();
    }
}

void PlaceOnBedButton::reload()
{
    const std::optional<Scene::OrientedBoundingBox>& bounding_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!bounding_box) {
        return;
    }

    const Domain::Vec3d h{bounding_box->dimensions * 0.5};

    const std::array<Domain::Vec3d, 8> offsets{
        Domain::Vec3d{-h.x(), -h.y(), -h.z()},
        Domain::Vec3d{-h.x(), -h.y(), h.z()},
        Domain::Vec3d{-h.x(), h.y(), -h.z()},
        Domain::Vec3d{-h.x(), h.y(), h.z()},
        Domain::Vec3d{h.x(), -h.y(), -h.z()},
        Domain::Vec3d{h.x(), -h.y(), h.z()},
        Domain::Vec3d{h.x(), h.y(), -h.z()},
        Domain::Vec3d{h.x(), h.y(), h.z()}
    };

    double min_z = std::numeric_limits<double>::max();

    for (const auto& offset : offsets) {
        const auto corner{bounding_box->center + bounding_box->rotation * offset};
        if (corner.z() < min_z) {
            min_z = corner.z();
        }
    }

    if (bounding_box && std::abs(min_z) > Domain::EPSILON) {
        set_visible(true);
        callbacks().action = [=, this]()
        {
            Domain::SquareMatrix4d relative_transform_world{Domain::SquareMatrix4d::Identity()};
            relative_transform_world.col(3).z() = -min_z;
            m_scene_interactor.transform_selection(relative_transform_world);
        };
        is_floating = true;
        return;
    }

    is_floating = false;
    set_visible(false);
    callbacks().action = []() {};
}

} // namespace Slic3r::App::Plater
