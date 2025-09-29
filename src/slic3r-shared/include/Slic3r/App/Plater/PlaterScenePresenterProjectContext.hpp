#pragma once

#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Plater/SinkingContours.hpp"

namespace Slic3r::App::Plater {

class PlaterScenePresenterProjectContext : public Scene::ScenePresenterProjectContext
{
public:
    PlaterScenePresenterProjectContext() = default;
    PlaterScenePresenterProjectContext(const PlaterScenePresenterProjectContext&) = delete;
    PlaterScenePresenterProjectContext& operator=(const PlaterScenePresenterProjectContext&) = delete;
    PlaterScenePresenterProjectContext(PlaterScenePresenterProjectContext&&) = default;

    SinkingContours& sinking_contours()
    {
        return m_sinking_contours;
    }

    const SinkingContours& sinking_contours() const
    {
        return m_sinking_contours;
    }

private:
    SinkingContours m_sinking_contours;
};

} // namespace Slic3r::App::Plater

