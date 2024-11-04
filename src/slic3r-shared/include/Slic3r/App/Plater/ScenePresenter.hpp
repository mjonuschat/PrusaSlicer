#pragma once

#include <unordered_map>
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/App/Plater/ScenePresenterProjectContext.hpp"

namespace Slic3r::App::Plater {

class ScenePresenter {
public:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, ScenePresenterProjectContext>;
private:
    ProjectContexts m_projects;
};

}
