#pragma once

#include "Slic3r/Biz/Arrange/Settings.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::Biz {

struct ArrangeBed
{
    const Domain::BedInstance& bed_instance;
    const double offset{};
};

using ArrangeBeds = std::vector<ArrangeBed>;

class ArrangeInteractor : public Biz::ISelectedProjectChangedListener
{
public:
    ArrangeInteractor(Scene::SceneInteractor& scene_interactor, const Domain::Workbench& workbench);

    void arrange(const Biz::Arrange::Settings& settings);

    void on_selected_project_changed(size_t index) override;

private:
    Scene::SceneInteractor& m_scene_interactor;
    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    const Domain::Workbench& m_workbench;

    ArrangeBeds get_beds(const double min_offset, const Biz::Arrange::Settings& settings) const;
};
} // namespace Slic3r::Biz
