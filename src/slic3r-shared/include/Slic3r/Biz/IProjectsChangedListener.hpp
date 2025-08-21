#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {

class IProjectsChangedListener {
public:
    virtual ~IProjectsChangedListener() = default;

    virtual void on_project_added(Domain::SelectionId project_id) {}
    virtual void on_project_will_be_removed(Domain::SelectionId project_id) {}
    virtual void on_project_removed(Domain::SelectionId project_id) {}
    virtual void on_project_changed(Domain::SelectionId project_id) {}
};

}
