#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {

class IProjectsChangedListener {
public:
    virtual ~IProjectsChangedListener() = default;

    virtual void on_project_added(Domain::SelectionId project_id) = 0;
    virtual void on_project_removed(Domain::SelectionId project_id) = 0;
    virtual void on_project_ids_swapped(Domain::SelectionId project_id1, Domain::SelectionId project_id2) = 0;
};

}
