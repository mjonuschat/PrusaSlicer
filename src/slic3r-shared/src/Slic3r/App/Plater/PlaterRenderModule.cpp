#include "Slic3r/App/Plater/PlaterRenderModule.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Plater/CameraGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/App/Plater/RotationGizmo.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include "imgui/imgui.h"
#include "Eigen/SVD"

#define ENABLED_DEBUG_IMGUI_FONT 1
#define ENABLED_DEBUG_IMGUI_ICONS 1

#if ENABLED_DEBUG_IMGUI_ICONS
#include <boost/nowide/convert.hpp>
#endif // ENABLED_DEBUG_IMGUI_ICONS

namespace Slic3r::App::Plater {

void PlaterRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    m_scene_presenter =
        std::make_unique<ScenePresenter>(m_workbench, m_project_interactor, *m_device);
    m_project_interactor.add_selected_project_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_selection_changed_listener(m_scene_presenter.get());
    init_gizmos();
    init_scene();
}

void PlaterRenderModule::init_scene()
{
    auto& scene_interactor = m_project_interactor.scene_interactor();

    const auto& bed = m_project_interactor.selected_project()
                          .config_containers()
                          .front()
                          ->bed_instances()
                          .front()
                          ->bed();

    const size_t x_size = 5;
    const size_t y_size = 5;
    const double span = 20;
    const double x_off = -((x_size - 1) * span) / 2 + bed.center().x();
    const double y_off = -((y_size - 1) * span) / 2 + bed.center().y();

    {
        scene_interactor.new_object_from_mesh(TriangleMesh{its_make_cube(10,10,10) });

        Biz::Scene::TransformMemento xform_memento;
        Transform3d xform = Transform3d::Identity();
        xform.translate(Vec3d{0 * span + x_off, 0 * span + y_off, 0});
        scene_interactor.transform_selection(xform.matrix(), xform_memento);

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 10, 10, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_cube(10,10,10)}, ModelVolumeType::NEGATIVE_VOLUME, xform.matrix());

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 0, -10, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_cube(10,10,10)}, ModelVolumeType::PARAMETER_MODIFIER, xform.matrix());

        xform = Transform3d::Identity();
        xform.translate(Vec3d{ 0, 5, 10});
        scene_interactor.add_volume_from_mesh(TriangleMesh{its_make_sphere(10, 12)}, ModelVolumeType::SUPPORT_ENFORCER, xform.matrix());

    }


    for (size_t x = 0; x < x_size; x++) {
        for (size_t y = 0; y < y_size; y++) {
            if (x == 0 && y == 0)
                continue;

            Transform3d xform = Transform3d::Identity();
            xform.translate(Vec3d{x * span + x_off, y * span + y_off, 0});
            scene_interactor.add_instance(xform.matrix());
        }
    }

    m_scene_presenter->scene().log_nodes();
}

void PlaterRenderModule::init_gizmos()
{
    m_gizmo_manager = std::make_unique<GizmoManager>(*m_device, *m_scene_presenter);
    m_gizmo_manager->add_base_gizmo<CameraGizmo>(*m_scene_presenter);
    m_gizmo_manager->add_base_gizmo<QuickSelectGizmo>(m_project_interactor.scene_interactor(), *m_device, *m_scene_presenter, m_screen_info);
    m_gizmo_manager->add_base_gizmo<QuickDragGizmo>(m_project_interactor.scene_interactor(), *m_scene_presenter);
    m_gizmo_manager->add_tool_gizmo<TranslationGizmo>(
        *m_device, m_gizmo_manager->data_factory(), *m_scene_presenter, m_project_interactor.scene_interactor()
    );
    m_gizmo_manager->add_tool_gizmo<RotationGizmo>(
        *m_device, m_gizmo_manager->data_factory(), *m_scene_presenter, m_project_interactor.scene_interactor()
    );
}


void PlaterRenderModule::render_scene()
{
    m_device->load_state();
    auto cmd_buffer = m_device->create_command_buffer();

    cmd_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer->set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer->clear_buffers(true, true);

    m_scene_presenter->render_scene(*cmd_buffer);

    m_gizmo_manager->render_scene(*cmd_buffer);

    cmd_buffer->submit();
}

class ImguiVecRender
{
public:
    void operator()(const char* label, const Vec2f& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }

    void operator()(const char* label, const Vec2d& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }
    //
    // void operator()(const char* label, const Vec3f& v)
    // {
    //     fill_data<3>(v);
    //     ImGui::InputFloat3(label, m_data);
    // }
    //
    void operator()(const char* label, const Vec3d& v)
    {
        fill_data<3>(v);
        ImGui::InputFloat3(label, m_data);
    }

    void operator()(const char* label, const Vec4f& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

    void operator()(const char* label, const Vec4d& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }
private:
    template <size_t N, typename VecT>
    void fill_data(const VecT &data)
    {
        for (size_t i = 0; i < N; i++) m_data[i] = static_cast<float>(data[i]);
    }
private:
    float m_data[4];
};

void imgui_scenegraph_node_info(const Scene::Node& node)
{
    ImGuiTreeNodeFlags node_flags = 0; //ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children().empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    const std::string& name = node.debug_name();
    if (ImGui::TreeNodeEx(
            &node, node_flags, "%s %s%s%s%s", name.empty() ? "Node" : name.c_str(),
            node.has_render_component() ? "(R)" : "", node.has_material_override() ? "(M)" : "",
            node.has_imgui_render_component() ? "(I)" : "", node.has_raycast_component() ? "(C)" : ""
        )) {

        static const Scene::Node* opened_node = nullptr;

        ImGui::SameLine();
        if (ImGui::SmallButton("info")) {
            opened_node = (opened_node == &node) ? nullptr : &node;
        }

        if (opened_node == &node) {
            auto transform{node.world_transform()};

            ImguiVecRender vec_render;
            for (size_t i = 0; i < 4; i++) {
                ImGui::PushID(i);
                vec_render("##", Vec4d{transform.row(i)});
                ImGui::PopID();
            }
        }

        for (const auto& ch : node.children()) {
            imgui_scenegraph_node_info(*ch);
        }
        ImGui::TreePop();
    }
}

void PlaterRenderModule::render_imgui()
{
    if (!m_scene_presenter->project_ready())
        return;

    trl.render(ImVec2(m_screen_info.logical_width(), m_screen_info.logical_height()));

    m_scene_presenter->render_imgui(m_screen_info);

    m_gizmo_manager->render_imgui();

    if (ImGui::Begin("Outline", &m_gui_win_open)) {
        ImGui::Text("Tool Gizmos");
        ToolType type = m_gizmo_manager->current_tool_type();
        if (type == ToolType::Translation) {
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.67f, 0.36f, 0.19f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.923f, 0.504f, 0.264f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.923f, 0.504f, 0.264f, 1.0f });
        }
        if (ImGui::Button("Translate"))
            // TODO: get and pass the correct printer type
            m_gizmo_manager->toggle_activate_tool(ToolType::Translation, ptFFF);
        if (type == ToolType::Translation)
            ImGui::PopStyleColor(3);
        if (type == ToolType::Rotation) {
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.67f, 0.36f, 0.19f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.923f, 0.504f, 0.264f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.923f, 0.504f, 0.264f, 1.0f });
        }
        if (ImGui::Button("Rotate"))
            // TODO: get and pass the correct printer type
            m_gizmo_manager->toggle_activate_tool(ToolType::Rotation, ptFFF);
        if (type == ToolType::Rotation)
            ImGui::PopStyleColor(3);
        ImGui::Separator();
        imgui_scenegraph_node_info(m_scene_presenter->scene().root());
    }
    ImGui::End();

#if ENABLED_DEBUG_IMGUI_FONT
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("Fonts test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Fonts", 2, ImGuiTableFlags_Borders)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Czech");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("oddělitelné");
            ImGui::Text("žádné otevřené kotvy");
            ImGui::Text("Přerušit");
            ImGui::Text("Přesné");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Russian");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Неизвестно");
            ImGui::Text("Внешний периметр");
            ImGui::Text("Нависающие периметры");
            ImGui::Text("Внутреннее заполнение");

            ImGui::EndTable();
        }
    }
    ImGui::End();
#endif // ENABLED_DEBUG_IMGUI_FONT

#if ENABLED_DEBUG_IMGUI_ICONS
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    if (ImGui::Begin("ImGui icons test/debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("Icons", 2, ImGuiTableFlags_Borders)) {
            float font_scale = ImGui::GetTextLineHeight() / 15.0f;
            int icon_sz = lround(16 * font_scale);

            int px = icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<std::wstring, std::string>> ICONS = {
                { std::wstring(&ImGui::PrintIconMarker),               "cog"                            },
                { std::wstring(&ImGui::PrinterIconMarker),             "printer"                        },
                { std::wstring(&ImGui::PrinterSlaIconMarker),          "sla_printer"                    },
                { std::wstring(&ImGui::FilamentIconMarker),            "spool"                          },
                { std::wstring(&ImGui::MaterialIconMarker),            "resin"                          },
                { std::wstring(&ImGui::MinimalizeButton),              "notification_minimalize"        },
                { std::wstring(&ImGui::MinimalizeHoverButton),         "notification_minimalize_hover"  },
                { std::wstring(&ImGui::RightArrowButton),              "notification_right"             },
                { std::wstring(&ImGui::RightArrowHoverButton),         "notification_right_hover"       },
                { std::wstring(&ImGui::PreferencesButton),             "notification_preferences"       },
                { std::wstring(&ImGui::PreferencesHoverButton),        "notification_preferences_hover" },
                { std::wstring(&ImGui::SliderFloatEditBtnIcon),        "edit_button"                    },
                { std::wstring(&ImGui::SliderFloatEditBtnPressedIcon), "edit_button_pressed"            },
                { std::wstring(&ImGui::ClipboardBtnIcon),              "copy_menu"                      },
                { std::wstring(&ImGui::ExpandBtn),                     "expand_btn"                     },
                { std::wstring(&ImGui::CollapseBtn),                   "collapse_btn"                   },
                { std::wstring(&ImGui::RevertButton),                  "undo"                           },
                { std::wstring(&ImGui::WarningMarkerSmall),            "notification_warning"           },
                { std::wstring(&ImGui::InfoMarkerSmall),               "notification_info"              },
                { std::wstring(&ImGui::PlugMarker),                    "plug"                           },
                { std::wstring(&ImGui::DowelMarker),                   "dowel"                          },
                { std::wstring(&ImGui::SnapMarker),                    "snap"                           },
                { std::wstring(&ImGui::HorizontalHide),                "horizontal_hide"                },
                { std::wstring(&ImGui::HorizontalShow),                "horizontal_show"                },
                { std::wstring(&ImGui::PrintIdle),                     "print_idle"                     },
                { std::wstring(&ImGui::PrintRunning),                  "print_running"                  },
                { std::wstring(&ImGui::PrintFinished),                 "print_finished"                 },
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::Button(boost::nowide::narrow(ICONS[i].first).c_str(), ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f);
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = int(1.25f * icon_sz);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons medium %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<std::wstring, std::string>> ICONS_MEDIUM = {
                { std::wstring(&ImGui::Lock),              "lock_closed"       },
                { std::wstring(&ImGui::LockHovered),       "lock_closed_f"     },
                { std::wstring(&ImGui::Unlock),            "lock_open"         },
                { std::wstring(&ImGui::UnlockHovered),     "lock_open_f"       },
                { std::wstring(&ImGui::DSRevert),          "undo"              },
                { std::wstring(&ImGui::DSRevertHovered),   "undo_f"            },
                { std::wstring(&ImGui::DSSettings),        "cog"               },
                { std::wstring(&ImGui::DSSettingsHovered), "cog_f"             },
                { std::wstring(&ImGui::ErrorTick),         "error_tick"        },
                { std::wstring(&ImGui::ErrorTickHovered),  "error_tick_f"      },
                { std::wstring(&ImGui::PausePrint),        "pause_print"       },
                { std::wstring(&ImGui::PausePrintHovered), "pause_print_f"     },
                { std::wstring(&ImGui::EditGCode),         "edit_gcode"        },
                { std::wstring(&ImGui::EditGCodeHovered),  "edit_gcode_f"      },
                { std::wstring(&ImGui::RemoveTick),        "colorchange_del"   },
                { std::wstring(&ImGui::RemoveTickHovered), "colorchange_del_f" },
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_medium", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_MEDIUM.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::Button(boost::nowide::narrow(ICONS_MEDIUM[i].first).c_str(), ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f);
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_MEDIUM[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = 2 * icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons large %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<std::wstring, std::string>> ICONS_LARGE = {
                { std::wstring(&ImGui::LegendTravel),            "legend_travel"                    },
                { std::wstring(&ImGui::LegendWipe),              "legend_wipe"                      },
                { std::wstring(&ImGui::LegendRetract),           "legend_retract"                   },
                { std::wstring(&ImGui::LegendDeretract),         "legend_deretract"                 },
                { std::wstring(&ImGui::LegendSeams),             "legend_seams"                     },
                { std::wstring(&ImGui::LegendToolChanges),       "legend_toolchanges"               },
                { std::wstring(&ImGui::LegendColorChanges),      "legend_colorchanges"              },
                { std::wstring(&ImGui::LegendPausePrints),       "legend_pauseprints"               },
                { std::wstring(&ImGui::LegendCustomGCodes),      "legend_customgcodes"              },
                { std::wstring(&ImGui::LegendCOG),               "legend_cog"                       },
                { std::wstring(&ImGui::LegendShells),            "legend_shells"                    },
                { std::wstring(&ImGui::LegendToolMarker),        "legend_toolmarker"                },
                { std::wstring(&ImGui::CloseNotifButton),        "notification_close"               },
                { std::wstring(&ImGui::CloseNotifHoverButton),   "notification_close_hover"         },
                { std::wstring(&ImGui::EjectButton),             "notification_eject_sd"            },
                { std::wstring(&ImGui::EjectHoverButton),        "notification_eject_sd_hover"      },
                { std::wstring(&ImGui::WarningMarker),           "notification_warning"             },
                { std::wstring(&ImGui::ErrorMarker),             "notification_error"               },
                { std::wstring(&ImGui::CancelButton),            "notification_cancel"              },
                { std::wstring(&ImGui::CancelHoverButton),       "notification_cancel_hover"        },
//                { std::wstring(&ImGui::SinkingObjectMarker),     "move"                              },
//                { std::wstring(&ImGui::CustomSupportsMarker),    "fdm_supports"                      },
//                { std::wstring(&ImGui::CustomSeamMarker),        "seam"                              },
//                { std::wstring(&ImGui::MmuSegmentationMarker),   "mmu_segmentation"                  },
//                { std::wstring(&ImGui::VarLayerHeightMarker),    "layers"                            },
                { std::wstring(&ImGui::DocumentationButton),      "notification_documentation"       },
                { std::wstring(&ImGui::DocumentationHoverButton), "notification_documentation_hover" },
                { std::wstring(&ImGui::InfoMarker),               "notification_info"                },
                { std::wstring(&ImGui::PlayButton),               "notification_play"                },
                { std::wstring(&ImGui::PlayHoverButton),          "notification_play_hover"          },
                { std::wstring(&ImGui::PauseButton),              "notification_pause"               },
                { std::wstring(&ImGui::PauseHoverButton),         "notification_pause_hover"         },
                { std::wstring(&ImGui::OpenButton),               "notification_open"                },
                { std::wstring(&ImGui::OpenHoverButton),          "notification_open_hover"          },
                { std::wstring(&ImGui::SlaViewOriginal),          "sla_view_original"                },
                { std::wstring(&ImGui::SlaViewProcessed),         "sla_view_processed"               },
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_large", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_LARGE.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::Button(boost::nowide::narrow(ICONS_LARGE[i].first).c_str(), ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f);
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_LARGE[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            px = 4 * icon_sz;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Icons extra large %dx%d", px, px);
            ImGui::TableSetColumnIndex(1);
            static const std::vector<std::pair<std::wstring, std::string>> ICONS_EXTRA_LARGE = {
                { std::wstring(&ImGui::ClippyMarker),          "notification_clippy"       },
                { std::wstring(&ImGui::SliceAllBtnIcon),       "slice_all"                 },
                { std::wstring(&ImGui::WarningMarkerDisabled), "notification_warning_grey" },
            };

            ImGui::PushItemWidth(200.0f);
            if (ImGui::BeginCombo("##icons_extra_large", nullptr, ImGuiComboFlags_HeightRegular)) {
                for (size_t i = 0; i < ICONS_EXTRA_LARGE.size(); ++i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.0f, 0.0f, 0.0f, 0.0f });
                    ImGui::Button(boost::nowide::narrow(ICONS_EXTRA_LARGE[i].first).c_str(), ImVec2(px, px) + ImGui::GetStyle().FramePadding * 2.0f);
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", ICONS_EXTRA_LARGE[i].second.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    }
    ImGui::End();
#endif // ENABLED_DEBUG_IMGUI_ICONS
}

void PlaterRenderModule::render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box)
{
    std::string node_name = "##node_hud_" + std::to_string(reinterpret_cast<long>(&n));

    ImGui::SetNextWindowPos({
        screen_bounding_box.max().x(),
        screen_bounding_box.min().y()
    });
    if (ImGui::Begin(node_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground)) {
        if (ImGui::SmallButton("Foc"))
            m_scene_presenter->scene().camera_trackball().set_focal_point({0, 0, 0});
    }
    ImGui::End();
}


void PlaterRenderModule::on_scene_mouse_event(
    const Platform::MouseEvent& e
)
{
    m_gizmo_manager->on_scene_mouse_event(e, m_screen_info);
}
void PlaterRenderModule::on_scene_keyboard_event(
    const Platform::KeyboardEvent& e
)
{
    m_gizmo_manager->on_scene_keyboard_event(e);
}

void PlaterRenderModule::on_activated()
{

}
void PlaterRenderModule::on_deactivated()
{

}
void PlaterRenderModule::on_screen_resized()
{
    //m_scene->camera().set_viewport(Render::Rect::from(0, 0, m_screen_info));
    auto viewport = Render::Rect::from(0, 0, m_screen_info);
    m_scene_presenter->screen_resized(viewport);
}


} // namespace Slic3r::App::Plater
