///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Biz/Platform/ListenerScope.hpp>

#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/IProjectsChangedListener.hpp"

#include <vector>

namespace Slic3r::Biz {

class ProjectInteractor;

class ObservableProjectList
    : public Biz::IObservableList<Domain::SelectionId>,
      public Biz::IProjectsChangedListener
{
public:
    explicit ObservableProjectList(Biz::ProjectInteractor& project_interactor);

    const Domain::SelectionId& at(size_t index) const override;

    size_t size() const override;

    void on_project_added(Domain::SelectionId project_id) override;
    void on_project_removed(Domain::SelectionId project_id) override;

private:
    Biz::ListenerScope<Biz::IProjectsChangedListener, Biz::ProjectInteractor, ObservableProjectList>
        m_project_changed_listener_scope;

    Biz::ProjectInteractor& m_project_interactor;
    using Projects = std::vector<std::unique_ptr<Domain::SelectionId>>;
    Projects m_projects;
};

} // namespace Slic3r::App
