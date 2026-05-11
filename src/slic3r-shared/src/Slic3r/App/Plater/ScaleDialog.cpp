#include "Slic3r/App/Plater/ScaleDialog.hpp"
#include "Slic3r/App/Plater/ScaleWidget.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Plater {

using Biz::_u8L;

ScaleDialog::ScaleDialog(Biz::ProjectInteractor& project_interactor) :
    GizmoWindow{_u8L("Scale"), Render::Icon::Scale}
{
    m_scale_widget = content()->emplace_back<ScaleWidget>(project_interactor);
}

void ScaleDialog::on_activated(Domain::SelectionId project_id)
{
    m_scale_widget->on_activated(project_id);
}

void ScaleDialog::on_deactivated()
{
    m_scale_widget->on_deactivated();
}

} // namespace Slic3r::App::Plater
