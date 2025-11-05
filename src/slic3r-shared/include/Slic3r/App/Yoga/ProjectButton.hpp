#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Biz/Platform/ListenerScope.hpp>

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App::Yoga {

class Text;
class LayoutButton;
class ProjectButtonBackground;

class ProjectButton
    : public AbstractButton,
      public Biz::DataObserver<Domain::SelectionId>,
      public Biz::ISelectedProjectChangedListener
{
public:
    ProjectButton(
        size_t index, const Domain::SelectionId& data, Biz::ProjectInteractor& project_interactor
    );

    size_t project_id() const;
    bool is_cross_hovered() const;

    bool is_selected();

    void on_selected_project_changed(size_t index) override;

    void on_view_will_be_removed() override;

protected:
    void set_selected(bool selected);

    void hovered_updated_internal() override;

    void on_data_update() override;

private:
    Biz::ProjectInteractor& m_project_interactor;
    ProjectButtonBackground* m_background{nullptr};
    Text* m_label{nullptr};
    LayoutButton* m_cross{nullptr};

    bool m_selected{false};
};

} // namespace Slic3r::App::Yoga
