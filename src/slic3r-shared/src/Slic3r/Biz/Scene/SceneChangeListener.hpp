#pragma once

namespace Slic3r::Biz::Scene {

class SceneChangeListener
{
public:
    virtual ~SceneChangeListener() = default;

    virtual void scene_changed() = 0;
};

}
