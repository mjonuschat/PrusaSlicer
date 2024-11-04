//
#pragma once
#include "libslic3r/TriangleMesh.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ListenerList.hpp"

namespace Slic3r::Biz::Scene {

class ISceneSelectionChangedListener
{
public:
    virtual ~ISceneSelectionChangedListener() = default;
    virtual void on_scene_selection_changed(Domain::SelectionId project_id, const Selection& selection) = 0;
};


class SceneInteractor final : public ISelectedProjectChangedListener
{
public:
    explicit SceneInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    void on_selected_project_changed(size_t index) override;
    void new_object_from_mesh(TriangleMesh &&mesh);

    const Selection& selection() const
    {
        ASSERT(m_selected_project_id != Domain::INVALID_ID);
        const auto it = m_projects.find(m_selected_project_id);
        ASSERT(it != m_projects.end());
        return it->second.selection;
    }

    void set_selection(const Selection& selection)
    {
        const auto it = m_projects.find(m_selected_project_id);
        ASSERT(it != m_projects.end());
        it->second.selection = selection;
        m_selection_changed_listeners.invoke([&](auto* l){
            l->on_scene_selection_changed(m_selected_project_id, selection);
        });
    }

    void add_scene_selection_changed_listener(ISceneSelectionChangedListener* l)
    {
        m_selection_changed_listeners.add(l);
    }

    void remove_scene_selection_changed_listener(ISceneSelectionChangedListener* l)
    {
        m_selection_changed_listeners.remove(l);
    }

private:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, SceneInteractorProjectContext>;

    Domain::Workbench& m_workbench;

    ProjectContexts m_projects;
    Domain::SelectionId m_selected_project_id {Domain::INVALID_ID};
    Biz::ListenerList<ISceneSelectionChangedListener> m_selection_changed_listeners;
};

} // namespace Slic3r::Biz::Interactor
