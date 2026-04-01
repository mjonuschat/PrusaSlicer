#pragma once

#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include <string>

namespace Slic3r::App::Platform {
class AbstractRenderModule;
} // namespace Slic3r::App::Platform

namespace Slic3r::App::Scene {
class GeometryDataFactory;
class ISceneProvider;
} // namespace Slic3r::App::Scene

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class MenuManager;
struct ThumbnailStore;
class Navigator;

class MenuCommandRegistrar
{
public:
    MenuCommandRegistrar(
        Platform::AbstractRenderModule& render_module,
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        ThumbnailStore& thumbnail_store
    );

    void register_top_bar_menus();
    void register_context_menus(
        Scene::GeometryDataFactory& data_factory,
        Scene::ISceneProvider* scene_provider
    );

private:

    void register_undo_redo_commands();
    void register_main_menu_commands();
    void register_main_menu_edit_commands();
    void register_main_menu_view_commands();
    void register_main_menu_config_commands();
    void register_main_menu_help_commands();

    void register_file_menu_commands();
    void register_file_menu_import_commands();
    void register_file_menu_export_commands();

    void register_bed_menu_commands();
    void register_bed_menu_add_shape_commands();

    void register_object_menu_commands();
    void register_object_menu_add_volume_commands();

    void register_instance_menu_commands();
    void register_svg_or_text_volume_menu_commands();
    void register_volume_menu_commands();
    void register_multi_object_menu_commands();

    void load_project();
    void save_project();
    void save_project_as();

    void load_object(Wildcards::TypeFlag specific_type = Wildcards::TypeFlag::None);

    void load_volume(
        Domain::ModelVolumeType type,
        Wildcards::TypeFlag specific_type = Wildcards::TypeFlag::None
    );

    void load_shape_from_gallery(
        Domain::ModelVolumeType type = Domain::ModelVolumeType::MODEL_PART
    );

    struct OpenBrowserParams
    {
        std::string url;
        bool force_remember_choice{true};
        bool is_localized_url{false};
    };

    void open_browser(OpenBrowserParams params);

    Platform::AbstractRenderModule& m_render_module;
    MenuManager& m_menu_manager;
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
    ThumbnailStore& m_thumbnail_store;
    Scene::GeometryDataFactory* m_data_factory{nullptr};
    Scene::ISceneProvider* m_scene_provider{nullptr};
};

} // namespace Slic3r::App
