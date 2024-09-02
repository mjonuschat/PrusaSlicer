#pragma once

#include "SelectionId.hpp"

namespace Slic3r::Biz {

class IProjectsChangedListener {
public:
    virtual ~IProjectsChangedListener() = default;

    virtual void on_project_added(SelectionId project_id) = 0;
    virtual void on_project_removed(SelectionId project_id) = 0;
    virtual void on_project_ids_swapped(SelectionId project_id1, SelectionId project_id2) = 0;
};

}
