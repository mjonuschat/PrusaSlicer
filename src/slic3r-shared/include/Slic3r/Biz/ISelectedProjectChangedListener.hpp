#pragma once

#include <cstddef>

namespace Slic3r::Biz {

class ISelectedProjectChangedListener
{
public:
    virtual ~ISelectedProjectChangedListener() = default;

    virtual void on_selected_project_changed(size_t index) = 0;
};

}
