#include "Slic3r/App/Scene/LightingHelper.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/Lights.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/Geometry.hpp"

#include <Slic3r/App/libvgcode/GCodeNodeTag.hpp>

#include <libslic3r/format.hpp>

#include <imgui/imgui.h>

namespace Slic3r::App::Scene {

void set_uniforms(const Lighting& lights, Render::Material& material)
{
    material.set_uniform("ambient_intensity", lights.ambient_intensity);
    material.set_uniform("num_lights", int(lights.lights.size()));
    for (size_t i = 0; i < lights.lights.size(); ++i) {
        const Light& light = lights.lights[i];
        std::string light_str = format("lights[%zu]", i);
        material
            .set_uniform(light_str + ".system", int(light.system))
            .set_uniform(light_str + ".direction", light.direction)
            .set_uniform(light_str + ".diffuse", light.diffuse)
            .set_uniform(light_str + ".shadows", light.shadows)
            .set_uniform(light_str + ".ambient", light.ambient)
            .set_uniform(light_str + ".specular", light.specular)
            .set_uniform(light_str + ".shininess", light.shininess);
    }
}

void set_uniforms(const PBRParams& pbr, Render::Material& material)
{
    material
        .set_uniform("material.metal", pbr.metal)
        .set_uniform("material.roughness", pbr.roughness)
        .set_uniform("material.ior", pbr.ior);
}

void render_imgui_scene_shading_customization(ISceneProvider& scene_provider, std::function<void(void)> cb_update_beds_shadows_data)
{
    Scene& scene = scene_provider.scene();

    float items_width = 150.0f;

    std::string caption = "Scene shading debug";
    const ImGuiStyle& style = ImGui::GetStyle();
    float min_w = ImGui::CalcTextSize(caption.c_str()).x +
        2.0f * (style.WindowPadding.x + style.FramePadding.x + style.ItemSpacing.x);
    ImGui::SetNextWindowSizeConstraints({ min_w, 0.0f }, { FLT_MAX, 0.75f * ImGui::GetMainViewport()->Size.y });
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Size * 0.5f, ImGuiCond_Once, {0.5f, 0.0f});
    if (ImGui::Begin(caption.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar)) {

        if (ImGui::CollapsingHeader("Shadows")) {

            bool bed_model_cast_shadow = scene.bed_model_cast_shadow();
            if (ImGui::Checkbox("Bed model cast shadow", &bed_model_cast_shadow)) {
                scene.set_bed_model_cast_shadow(bed_model_cast_shadow);
                if (cb_update_beds_shadows_data != nullptr)
                    cb_update_beds_shadows_data();
            }

            if (ImGui::BeginTable("Shadows", 2, ImGuiTableFlags_Borders)) {

                int shadowsmap_size = scene.shadowsmap_size();
                if (shadowsmap_size > 0) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Shadowsmap size");
                    ImGui::TableSetColumnIndex(1);

                    const std::vector<int> sizes = {
                        512,
                        1024,
                        2048,
                        4096,
                        8192
                    };

                    std::vector<std::string> sizes_str;
                    std::transform(sizes.begin(), sizes.end(), std::back_inserter(sizes_str), [](int size) {
                        return std::to_string(size) + "x" + std::to_string(size);
                    });

                    auto it = std::find(sizes.begin(), sizes.end(), shadowsmap_size);
                    DEBUG_ASSERT(it != sizes.end());
                    int sel_size = int(std::distance(sizes.begin(), it));

                    const char* preview_value = sizes_str[sel_size].c_str();

                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::BeginCombo("##sizes", preview_value)) {
                        for (int i = 0; i < int(sizes_str.size()); i++) {
                            bool is_selected = (sel_size == i);
                            if (ImGui::Selectable(sizes_str[i].c_str(), is_selected))
                                sel_size = i;

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    if (sizes[sel_size] != shadowsmap_size)
                        scene.set_shadowsmap_size(sizes[sel_size]);
                }

                float intensity = scene.shadows_intensity();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Intensity");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(items_width);
                if (ImGui::SliderFloat("##intensity", &intensity, 0.2f, 1.0f, "%.2f", ImGuiSliderFlags_NoInput))
                    scene.set_shadows_intensity(intensity);

                ImGui::EndTable();
            }

            if (ImGui::Button("Default##Shadows"))
                scene.set_default_shadows_intensity();
        }

        if (ImGui::CollapsingHeader("Ambient occlusion")) {

            Domain::Index2 fb_size = scene.ao_framebuffer_size();
            if (fb_size[0] > 0 && fb_size[1] > 0) {
                if (ImGui::BeginTable("AO", 2, ImGuiTableFlags_Borders)) {

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Framebuffer size");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%dx%d", fb_size[0], fb_size[1]);

                    size_t k_size = scene.ao_kernel_size();
                    if (k_size > 0) {

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Kernel size");
                        ImGui::TableSetColumnIndex(1);

                        const std::vector<int> k_sizes = {
                            4,
                            8,
                            16,
                            32,
                            64,
                            128,
                            256,
                        };

                        std::vector<std::string> k_sizes_str;
                        std::transform(k_sizes.begin(), k_sizes.end(), std::back_inserter(k_sizes_str), [](int size) {
                            return std::to_string(size);
                        });

                        auto k_it = std::find(k_sizes.begin(), k_sizes.end(), k_size);
                        DEBUG_ASSERT(k_it != k_sizes.end());
                        int sel_k_size = int(std::distance(k_sizes.begin(), k_it));

                        const char* k_preview_value = k_sizes_str[sel_k_size].c_str();

                        ImGui::SetNextItemWidth(items_width);
                        if (ImGui::BeginCombo("##k_sizes", k_preview_value)) {
                            for (int i = 0; i < int(k_sizes_str.size()); i++) {
                                bool is_selected = (sel_k_size == i);
                                if (ImGui::Selectable(k_sizes_str[i].c_str(), is_selected))
                                    sel_k_size = i;

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        if (k_sizes[sel_k_size] != k_size)
                            scene.set_ao_kernel_size(k_sizes[sel_k_size]);
                    }

                    size_t n_size = scene.ao_noise_size();
                    if (n_size > 0) {

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Noise size");
                        ImGui::TableSetColumnIndex(1);

                        const std::vector<int> n_sizes = {
                            4,
                            8,
                            16,
                            32,
                        };

                        std::vector<std::string> n_sizes_str;
                        std::transform(n_sizes.begin(), n_sizes.end(), std::back_inserter(n_sizes_str), [](int size) {
                            return std::to_string(size) + "x" + std::to_string(size);
                        });

                        auto n_it = std::find(n_sizes.begin(), n_sizes.end(), n_size);
                        DEBUG_ASSERT(n_it != n_sizes.end());
                        int sel_n_size = int(std::distance(n_sizes.begin(), n_it));

                        const char* n_preview_value = n_sizes_str[sel_n_size].c_str();

                        ImGui::SetNextItemWidth(items_width);
                        if (ImGui::BeginCombo("##n_sizes", n_preview_value)) {
                            for (int i = 0; i < int(n_sizes_str.size()); i++) {
                                bool is_selected = (sel_n_size == i);
                                if (ImGui::Selectable(n_sizes_str[i].c_str(), is_selected))
                                    sel_n_size = i;

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        if (n_sizes[sel_n_size] != n_size)
                            scene.set_ao_noise_size(n_sizes[sel_n_size]);
                    }

                    float radius = scene.ao_radius();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##ao_radius", &radius, 0.1f, 50.0f, "%.1f", ImGuiSliderFlags_NoInput))
                        scene.set_ao_radius(radius);

                    float bias = scene.ao_bias();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Bias");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##ao_bias", &bias, 0.001f, 10.0f, "%.3f", ImGuiSliderFlags_NoInput))
                        scene.set_ao_bias(bias);

                    size_t bf_size = scene.ao_blur_filter_size();
                    if (bf_size > 0) {

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Blur filter size");
                        ImGui::TableSetColumnIndex(1);

                        const std::vector<int> bf_sizes = {
                            2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
                        };

                        std::vector<std::string> bf_sizes_str;
                        std::transform(bf_sizes.begin(), bf_sizes.end(), std::back_inserter(bf_sizes_str), [](int size) {
                            return std::to_string(size) + "x" + std::to_string(size);
                        });

                        auto bf_it = std::find(bf_sizes.begin(), bf_sizes.end(), bf_size);
                        DEBUG_ASSERT(bf_it != bf_sizes.end());
                        int sel_bf_size = int(std::distance(bf_sizes.begin(), bf_it));

                        const char* bf_preview_value = bf_sizes_str[sel_bf_size].c_str();

                        ImGui::SetNextItemWidth(items_width);
                        if (ImGui::BeginCombo("##bf_sizes", bf_preview_value)) {
                            for (int i = 0; i < int(bf_sizes_str.size()); i++) {
                                bool is_selected = (sel_bf_size == i);
                                if (ImGui::Selectable(bf_sizes_str[i].c_str(), is_selected))
                                    sel_bf_size = i;

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        if (bf_sizes[sel_bf_size] != bf_size)
                            scene.set_ao_blur_filter_size(bf_sizes[sel_bf_size]);
                    }

                    ImGui::EndTable();
                }

                if (ImGui::Button("Default##ao")) {
                    scene.set_default_ao_kernel_size();
                    scene.set_default_ao_noise_size();
                    scene.set_default_ao_radius();
                    scene.set_default_ao_bias();
                    scene.set_default_ao_blur_filter_size();
                }
            }
        }

        if (ImGui::CollapsingHeader("Physically based rendering")) {

            if (ImGui::BeginTable("PBR", 2, ImGuiTableFlags_Borders)) {

                float intensity = scene.pbr_intensity();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Intensity");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(items_width);
                if (ImGui::SliderFloat("##pbr_intensity", &intensity, 1.0f, 100.0f, "%.1f", ImGuiSliderFlags_NoInput))
                    scene.set_pbr_intensity(intensity);

                ImGui::EndTable();
            }

            if (ImGui::Button("Default##pbr"))
                scene.set_default_pbr_intensity();

            Node::NodeList nodes;
            scene.root().query([](const Node* n) {
                return n->has_render_component() && n->render_component()->has_pbr();
            }, nodes);

            if (!nodes.empty()) {
                if (ImGui::BeginTable("PBR-scene", 5, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                    ImGui::TableSetupColumn("Node");
                    ImGui::TableSetupColumn("Metal");
                    ImGui::TableSetupColumn("Roughness");
                    ImGui::TableSetupColumn("IOR");
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < nodes.size(); ++i) {
                        Node* n = nodes[i];
                        PBRParams pbr = *n->render_component()->pbr();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        std::string name = n->debug_name();
                        const Plater::SceneNodeTag* tag = n->tag_of_type<Plater::SceneNodeTag>();
                        if (tag != nullptr) {
                            name += " (";
                            name += std::to_string(tag->object_id);
                            name += ", " + std::to_string(tag->instance_id);
                            name += ", " + std::to_string(tag->volume_id);
                            name += ")";
                        }
                        ImGui::Text("%s", name.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(items_width);
                        float metal = pbr.metal;
                        if (ImGui::SliderFloat(format("##metal%zu", i).c_str(), &metal, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_NoInput))
                            n->render_component()->set_pbr({ metal, pbr.roughness, pbr.ior});

                        ImGui::TableSetColumnIndex(2);
                        ImGui::SetNextItemWidth(items_width);
                        float roughness = pbr.roughness;
                        if (ImGui::SliderFloat(format("##roughness%zu", i).c_str(), &roughness, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_NoInput))
                            n->render_component()->set_pbr({ pbr.metal, roughness, pbr.ior });

                        ImGui::TableSetColumnIndex(3);
                        ImGui::SetNextItemWidth(items_width);
                        float ior = pbr.ior;
                        if (ImGui::SliderFloat(format("##ior%zu", i).c_str(), &ior, 1.0f, 2.5f, "%.3f", ImGuiSliderFlags_NoInput))
                            n->render_component()->set_pbr({ pbr.metal, pbr.roughness, ior });

                        ImGui::TableSetColumnIndex(4);
                        if (ImGui::Button(format("Default##pbr%zu", i).c_str())) {
                            bool found = false;
                            const Plater::SceneNodeTag* sn_tag = n->tag_of_type<Plater::SceneNodeTag>();
                            if (sn_tag != nullptr) {
                                pbr = DEFAULT_VOLUME_PBRPARAMS;
                                found = true;
                            }
                            else {
                                const BedNodeTag* bn_tag = n->tag_of_type<BedNodeTag>();
                                if (bn_tag != nullptr) {
                                    pbr = (bn_tag->type == BedElementType::Model) ?
                                        DEFAULT_BED_MODEL_PBRPARAMS : DEFAULT_BED_PLATE_PBRPARAMS;
                                    found = true;
                                }
                                else {
                                    const libvgcode::GCodeNodeTag* gcn_tag = n->tag_of_type<libvgcode::GCodeNodeTag>();
                                    if (gcn_tag != nullptr){
                                        pbr = (gcn_tag->type == libvgcode::GCodeElementType::Toolpaths) ?
                                          DEFAULT_GCODE_SEGMENTS_PBRPARAMS : DEFAULT_GCODE_OPTIONS_PBRPARAMS;
                                        found = true;
                                    }
                                }
                            }
                            if (found)
                                n->render_component()->set_pbr(pbr);
                        }
                    }

                    ImGui::EndTable();
                }
            }
        }
    }
    ImGui::End();
}

static std::pair<float, float> xyz_to_az(const Domain::Vec3f& xyz)
{
    if (xyz.isApprox(Domain::Vec3f::UnitZ()))
        return { 0.0f, 0.0f };
    else if (xyz.isApprox(-Domain::Vec3f::UnitZ()))
        return { 0.0f, float(M_PI) };

    std::pair<float, float> za = Biz::Algorithms::Geometry::dir_to_spheric(xyz);
    if (za.second < 0.0f)
        za.second += 2.0f * float(M_PI);
    return { za.second, za.first };
}

void render_imgui_lights_customization(ISceneProvider& scene_provider)
{
    Scene& scene = scene_provider.scene();
    Lighting lights = scene.lights();

    float items_width = 150.0f;
    bool modified = false;
    static int edit_id = -1;

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Size * 0.5f, ImGuiCond_Once, {0.5f, 1.0f});
    if (ImGui::Begin("Lights customization", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar)) {
        bool pbr_enabled = Scene::Scene::graphics_settings().pbr_enabled();
        bool shadows_enabled = Scene::Scene::graphics_settings().shadows_enabled();
        if (edit_id == -1) {

            if (pbr_enabled) {
                if (ImGui::BeginTable("Lighting options", 2, ImGuiTableFlags_Borders)) {

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ambient intensity");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##ambient", &lights.ambient_intensity, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_NoInput))
                        modified = true;

                    ImGui::EndTable();
                }
            }

            int columns = pbr_enabled ? 7 : 10;
            if (shadows_enabled)
                ++columns;
            if (ImGui::BeginTable("Lights", columns, ImGuiTableFlags_Borders)) {

                ImGui::TableSetupColumn("Light");
                ImGui::TableSetupColumn("Ref.System");
                ImGui::TableSetupColumn("Azimuth");
                ImGui::TableSetupColumn("Zenith");
                if (!pbr_enabled)
                    ImGui::TableSetupColumn("Ambient");
                ImGui::TableSetupColumn("Diffuse");
                if (!pbr_enabled) {
                    ImGui::TableSetupColumn("Specular");
                    ImGui::TableSetupColumn("Shininess");
                }
                if (shadows_enabled)
                    ImGui::TableSetupColumn("Shadows");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < lights.lights.size(); ++i) {
                    int col_id = 0;
                    Light& l = lights.lights[i];

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(col_id++);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Light #%zu", i + 1);
                    ImGui::TableSetColumnIndex(col_id++);
                    ImGui::Text("%s", to_string(l.system).c_str());

                    std::pair<float, float> az = xyz_to_az(l.direction);
                    ImGui::TableSetColumnIndex(col_id++);
                    ImGui::Text("%.3f", rad2deg(az.first));
                    ImGui::TableSetColumnIndex(col_id++);
                    ImGui::Text("%.3f", rad2deg(az.second));
                    if (!pbr_enabled) {
                        ImGui::TableSetColumnIndex(col_id++);
                        ImGui::Text("%.3f", l.ambient);
                    }
                    ImGui::TableSetColumnIndex(col_id++);
                    ImGui::Text("%.3f", l.diffuse);
                    if (!pbr_enabled) {
                        ImGui::TableSetColumnIndex(col_id++);
                        ImGui::Text("%.3f", l.specular);
                        ImGui::TableSetColumnIndex(col_id++);
                        ImGui::Text("%.3f", l.shininess);
                    }

                    if (shadows_enabled) {
                        ImGui::TableSetColumnIndex(col_id++);
                        ImGui::Text("%s", l.shadows ? "Yes" : "No");
                    }

                    ImGui::TableSetColumnIndex(col_id++);
                    if (ImGui::Button(fmt::format("Edit##{}", i).c_str()))
                        edit_id = int(i);

                    bool remove_enabled = lights.lights.size() > 1;
                    if (!remove_enabled)
                        ImGui::BeginDisabled();
                    ImGui::TableSetColumnIndex(col_id++);
                    if (ImGui::Button(fmt::format("Remove##{}", i).c_str())) {
                        lights.lights.erase(lights.lights.begin() + i);
                        modified = true;
                    }
                    if (!remove_enabled)
                        ImGui::EndDisabled();
                }

                ImGui::EndTable();
            }

            bool add_enabled = lights.lights.size() < MAX_NUM_LIGHTS;
            if (!add_enabled)
                ImGui::BeginDisabled();

            if (ImGui::Button("Add")) {
                lights.lights.push_back(Light());
                modified = true;
            }

            if (!add_enabled)
                ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Default")) {
                lights.lights = DEFAULT_LIGHTS;
                lights.ambient_intensity = DEFAULT_LIGHT_AMBIENT;
                modified = true;
            }
        }
        else {
            ImGui::Text("Light #%d", edit_id + 1);

            Light& l = lights.lights[edit_id];

            if (ImGui::BeginTable("Light", 2, ImGuiTableFlags_Borders)) {

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Reference system");
                ImGui::TableSetColumnIndex(1);

                std::vector<std::string> systems;
                systems.reserve(LIGHT_REFERENCE_SYSTEMS_COUNT);
                for (size_t i = 0; i < LIGHT_REFERENCE_SYSTEMS_COUNT; ++i) {
                    systems.emplace_back(to_string(LightReferenceSystem(i)));
                }

                const std::string& system = to_string(l.system);

                auto it = std::find(systems.begin(), systems.end(), system);
                DEBUG_ASSERT(it != systems.end());
                int sel = int(std::distance(systems.begin(), it));

                const char* preview_value = system.c_str();

                ImGui::SetNextItemWidth(items_width);
                if (ImGui::BeginCombo("##refsystem", preview_value)) {
                    for (int i = 0; i < int(systems.size()); i++) {
                        bool is_selected = (sel == i);
                        if (ImGui::Selectable(systems[i].c_str(), is_selected)) {
                            sel = i;
                            if (systems[sel] == "World") {
                                l.direction = scene.camera().model().block<3, 3>(0,0).cast<float>() * l.direction;
                                float nn = l.direction.norm();
                                DEBUG_ASSERT(std::abs(l.direction.norm() - 1.0f) < FLT_EPSILON);
                            }
                            else if (systems[sel] == "Camera") {
                                l.direction = scene.camera().view().block<3, 3>(0, 0).cast<float>() * l.direction;
                                float nn = l.direction.norm();
                                DEBUG_ASSERT(std::abs(l.direction.norm() - 1.0f) < FLT_EPSILON);
                            }
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (systems[sel] != system) {
                    l.system = LightReferenceSystem(sel);
                    modified = true;
                }

                std::pair<float, float> az = xyz_to_az(l.direction);
                bool az_modified = false;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Azimuth");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(items_width);
                ImGui::Text("0°");
                ImGui::SameLine();
                float azimuth = rad2deg(az.first);
                if (ImGui::SliderFloat("##azimuth", &azimuth, 0.0f, 360.0f, "%.2f", ImGuiSliderFlags_NoInput))
                    az_modified = true;
                ImGui::SameLine();
                ImGui::Text("360°");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Zenith");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(items_width);
                ImGui::Text("0°");
                ImGui::SameLine();
                float zenith = rad2deg(az.second);
                if (ImGui::SliderFloat("##zenith", &zenith, 0.0f, 180.0f, "%.2f", ImGuiSliderFlags_NoInput))
                    az_modified = true;
                ImGui::SameLine();
                ImGui::Text("180°");

                if (az_modified) {
                    l.direction = Biz::Algorithms::Geometry::spheric_to_dir(deg2rad(zenith), deg2rad(azimuth)).cast<float>();
                    modified = true;
                }

                if (!pbr_enabled) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Ambient");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##ambient", &l.ambient, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_NoInput))
                        modified = true;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Diffuse");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(items_width);
                if (ImGui::SliderFloat("##diffuse", &l.diffuse, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_NoInput))
                    modified = true;

                if (!pbr_enabled) {

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Specular");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##specular", &l.specular, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_NoInput))
                        modified = true;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Shininess");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(items_width);
                    if (ImGui::SliderFloat("##shininess", &l.shininess, 0.0f, 200.0f, "%.2f", ImGuiSliderFlags_NoInput))
                        modified = true;
                }

                if (shadows_enabled){
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Cast shadows");
                    ImGui::TableSetColumnIndex(1);

                    if (ImGui::Checkbox("##shadows", &l.shadows)) {
                        if (l.shadows) {
                            for (size_t i = 0; i < lights.lights.size(); ++i) {
                                if (i == edit_id)
                                    continue;
                                lights.lights[i].shadows = false;
                            }
                        }
                        else if (lights.lights.size() == 2) {
                            for (size_t i = 0; i < lights.lights.size(); ++i) {
                                if (i != edit_id)
                                    lights.lights[i].shadows = true;
                            }
                        }
                        modified = true;
                    }
                }

                ImGui::EndTable();
            }

            ImGui::Separator();
            if (ImGui::Button("Close"))
                edit_id = -1;
        }
    }
    ImGui::End();

    if (modified) {
        // ensure at least one light casts shadows
        auto it = std::find_if(lights.lights.begin(), lights.lights.end(), [](const Light& l) {
            return l.shadows;
        });
        if (it == lights.lights.end())
            lights.lights.front().shadows = true;

        scene.set_lights(lights);
    }
}

} // namespace Slic3r::App::Scene