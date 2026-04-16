#pragma once

#include "Slic3r/App/Yoga/GizmoWindow.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Plater {
class PlaceOnBedButton;
class ScaleWidget;

class ScaleDialog final : public Yoga::GizmoWindow
{
public:
    ScaleDialog(Biz::ProjectInteractor& project_interactor);

    void on_activated(Domain::SelectionId project_id);
    void on_deactivated();

private:
    ScaleWidget* m_scale_widget{nullptr};
};
} // namespace Slic3r::App::Plater
