#pragma once

namespace Slic3r::Biz::Scene {

class SceneSelectionChangeListener
{
public:
    virtual ~SceneSelectionChangeListener() = default;
    virtual void scene_selection_changed() = 0;
};

}
