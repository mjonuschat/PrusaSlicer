///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/TextGizmo.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Plater/TextDialog.hpp"
#include <Slic3r/App/Plater/SceneNodeTag.hpp>
#include <Slic3r/App/AppServices.hpp>
#include <Slic3r/App/Scene/NodeVisitor.hpp> // visit_conditional_transform()

#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <boost/nowide/convert.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h> // using std::string for inputs

#include <Slic3r/Directories.hpp>
#include <Slic3r/Domain/TriangleMesh.hpp>
#include <Slic3r/Domain/ModelObject.hpp> // add volume into object
#include <Slic3r/Biz/Algorithms/TriangleMesh.hpp>
#include <Slic3r/Biz/Emboss/Emboss.hpp> // also copy in libslic3r for SurfaceCut
#include <Slic3r/Biz/Emboss/EmbossJob.hpp> // embossing jobs
#include <Slic3r/Biz/Platform/PlatformServices.hpp> // main_thread_dispatcher
#include "libslic3r/Utils.hpp"

namespace {
using namespace Slic3r;

struct Drag {
    // Project interactor transformation cache;
    Biz::Scene::TransformMemento memento;

    // volume world transformation before draggig
    Domain::Transform3d to_world;
    Domain::Transform3d instance;
    Domain::Transform3d instance_inv;
    Domain::Transform3d volume_inv;

    // screen coordinate volume center, change on the mouse drag(move)
    Domain::Vec2d volume_center;

    // screen coordinate offset volume center from mouse at drag start
    // fixed during dragging
    Domain::Vec2d volume_offset;

    Domain::SquareMatrix4d last_tr = Domain::SquareMatrix4d::Identity(); // for rendring

    // True on hit object surface otherwise false. (cross hair color)
    bool valid = true;
};
} // namespace

namespace Slic3r::Biz::Emboss {
// TODO: made shape by current selected preset and text
class TextShapeProvider : public ShapeProvider
{
public:
    TextShapeProvider(
        const Domain::TextConfiguration& text_configuration,
        const Domain::EmbossProjection& projection,
        const Biz::Emboss::TextLines& text_lines,
        FontFileWithCache& font_with_cache
    ) 
        : m_text_configuration(text_configuration)
        , m_font_with_cache(font_with_cache)
    {
        m_shape.projection = projection; // copy current projection
        m_text_lines = text_lines; // copy
    }

    Domain::EmbossShape& get_shape() override
    {
        if (!m_shape.final_shape.expolygons.empty())
            return m_shape; // use cached value
                
        std::wstring text = boost::nowide::widen(m_text_configuration.text);
        const Domain::FontProp& font_prop = m_text_configuration.style.prop;
        m_shape.shapes_with_ids = text2vshapes(m_font_with_cache, text, font_prop);
        const Domain::FontFile& ff = *m_font_with_cache.font_file;
        m_shape.scale = get_text_shape_scale(font_prop, ff);
        return m_shape;
    }

    void write(Domain::ModelVolume& volume) const override
    {
        ShapeProvider::write(volume);
        volume.text_configuration = m_text_configuration; // copy
        ASSERT(volume.emboss_shape.has_value());

        // Fix for object: stored attribute that volume is embossed per glyph when it is object
        // Removing object without text gizmo
        if (volume.is_the_only_one_part()) {
            if (m_text_configuration.style.prop.per_glyph)
                volume.text_configuration->style.prop.per_glyph = false;
            if (m_shape.projection.use_surface)
                volume.emboss_shape->projection.use_surface = false;
        }
    }

private:
    // font item is not used for create object
    Domain::TextConfiguration m_text_configuration;
    FontFileWithCache& m_font_with_cache;
};
} // namespace Slic3r::Biz::Emboss

namespace {
template<typename T>
bool set_opt(std::optional<T>& val_opt, double new_value, double scale) {
    T scaled_new = static_cast<T>(new_value / scale);
    if (scaled_new == val_opt.value_or(T(0)))
        return false; // no change
    else if (scaled_new == 0)
        val_opt.reset();
    else
        val_opt = scaled_new;
    return true;
}

// Stamp for Scene object -> Node
struct EmbossTag {};
struct CrossHairTag: EmbossTag {};

// work only with selected Text volume/object
// NOTE: move function near SceneInteractor::transform_selection
void transform_selection_relative(const Domain::Transform3d& tr, Biz::ProjectInteractor& project_interactor)
{
    // get current transformation of the volume
    const Domain::Project& project = project_interactor.selected_project();
    Biz::Scene::SceneInteractor& scene_interactor = project_interactor.scene_interactor();
    const Domain::ElementRef& el = scene_interactor.object_selection().elements.front();
    Domain::Transform3d instance_tr = project.find_instance_by_id(el.object_id, el.instance_id)->get_matrix();
    const Domain::ModelVolume& volume = (el.volume_id != 0) ?
        *project.find_volume_by_id(el.object_id, el.volume_id) :
        *project.find_object_by_id(el.object_id)->volumes.front();
    Domain::Transform3d volume_tr = volume.get_matrix();
    Domain::Transform3d world_relative = instance_tr * volume_tr * tr * volume_tr.inverse() * instance_tr.inverse();
    scene_interactor.transform_selection(world_relative.matrix());
}

double mm_to_inch = 25.4;
double inch_to_mm = 1. / mm_to_inch;
double UP_LIMIT = 0.9;

Domain::Transform3d get_volume_transformation(Domain::Transform3d, const Domain::Vec3d&, const Domain::Vec3d&, const Domain::Transform3d&, const std::optional<float>&, const std::optional<float>&, const std::optional<double>& );
} // namespace

namespace Slic3r::App::Plater {

namespace {
void set_dialog_rotation(TextDialog& dialog, const Biz::Emboss::TextPresetManager& preset_manager, bool use_deg);    
} // namespace

struct TextGizmo::Drag : public ::Drag {};

TextGizmo::TextGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor& project_interactor,
    Biz::Emboss::IFontManager& font_manager,
    Scene::GizmoManager& gizmo_manager
) :
    m_device(device),
    m_scene_presenter(scene_presenter),
    m_project_interactor(project_interactor),
    m_font_manager(font_manager),
    m_gizmo_manager(gizmo_manager),
    m_preset_manager(
        font_manager,
        ImGui::GetIO().Fonts->GetGlyphRangesDefault(),
        Slic3r::data_dir() + "/text_emboss_presets.cereal"
    ),
    m_text_lines(m_preset_manager, project_interactor, scene_presenter, device),
    m_drag(nullptr),
    m_last_loaded_volume_id(0) // invalid value
{
    // Initialize font descriptor to font copied with application
    m_preset_manager.get_preset().emboss_style.descriptor = Domain::FontDescriptor{
        .name = "Prusa-slic3r font",
        .path = Slic3r::resources_dir() + "/fonts/NotoSans-Regular.ttf",
        .type = Domain::FontDescriptor::Type::file_path
    };

    // Dialog callback settings (order follow UI)
    m_dialog = std::make_unique<TextDialog>();
    m_dialog->callbacks().text_changed = [this](const std::string& text) {
        if (m_preset_manager.get_font_prop().per_glyph) { 
            // update text lines when change count of lines
            unsigned prev_count_lines = Biz::Emboss::get_count_lines(m_text);
            unsigned now_count_lines = Biz::Emboss::get_count_lines(text);
            if (prev_count_lines != now_count_lines)
                m_text_lines.create_text_lines(now_count_lines);
        }
        m_dialog->set_enable_line_gap(Biz::Emboss::get_count_lines(text) > 1);
        m_text = text; 
        update_volume();
    };
    m_dialog->callbacks().font_selection_changed = [this](const Domain::FontDescriptor& font_descriptor){
        m_preset_manager.set_font(font_descriptor); 
        update_volume();
    }; 
    // NOTE: style is only subcategory of font    
    m_dialog->callbacks().height_changed = [this](double value) {
        m_preset_manager.get_font_prop().size_in_mm = m_use_inch? value * inch_to_mm : value;
        if (m_preset_manager.get_font_prop().per_glyph) // change size of line visualization
            m_text_lines.create_text_lines(m_text_lines.get_lines().size());
        update_volume();
    };
    m_dialog->callbacks().depth_changed = [this](double value) {
        m_preset_manager.get_preset().projection.depth = m_use_inch ? value * inch_to_mm : value;
        update_volume();
    };

    // Advanced settings
    m_dialog->callbacks().use_surface_checked = [this](bool check) {
        m_preset_manager.get_preset().projection.use_surface = check;
        update_volume();
    };
    m_dialog->callbacks().per_glyph_checked = [this](bool check) {
        m_preset_manager.get_font_prop().per_glyph = check;
        if (check) {
            m_text_lines.create_text_lines(Biz::Emboss::get_count_lines(m_text));
        } else {
            m_text_lines.reset();
        }
        update_volume();
    };
    m_dialog->callbacks().align_changed = [this](const Domain::FontProp::Align& align) {
        m_preset_manager.get_font_prop().align = align;
        if (m_preset_manager.get_font_prop().per_glyph) // change position of the line visualization
            m_text_lines.create_text_lines(m_text_lines.get_lines().size());
        update_volume();
    };
    auto set_optional = [this](std::optional<int>& val_opt, double new_value, double scale) {
        if (set_opt(val_opt, new_value, scale)) {        
            m_preset_manager.clear_glyphs_cache();
            update_volume();
        }
    };
    m_dialog->callbacks().char_gap_changed = [this, set_optional](double value) {
        set_optional(m_preset_manager.get_font_prop().char_gap, value, m_volume_scale.char_gap);
    };
    m_dialog->callbacks().line_gap_changed = [this, set_optional](double value) {
        set_optional(m_preset_manager.get_font_prop().line_gap, value, m_volume_scale.line_gap);
    };
    m_dialog->callbacks().boldness_changed = [this](double value) {
        if (set_opt(m_preset_manager.get_font_prop().boldness, value, m_volume_scale.char_gap)) {
            m_preset_manager.clear_glyphs_cache();
            update_volume();        
        }
    };
    m_dialog->callbacks().skew_ratio_changed = [this](double value) {
        if (set_opt(m_preset_manager.get_font_prop().skew, value, 1.)) {
            m_preset_manager.clear_glyphs_cache();
            update_volume();
        }
    };
    m_dialog->callbacks().surface_distance_changed = [this](double value_unit) { 
        double value = m_use_inch ? value_unit * inch_to_mm: value_unit;
        std::optional<float>& distance = m_preset_manager.get_preset().distance;
        double diff = value - distance.value_or(0.f);
        if (Domain::is_approx(diff, 0., 1e-3))
            return; // no change

        Domain::Transform3d relative_volume_tr{ Eigen::Translation3d(Domain::Vec3d(0., 0., diff)) };
        transform_selection_relative(relative_volume_tr, m_project_interactor);

        // calculate current surface distance
        const Domain::Project& project = m_project_interactor.selected_project();
        const Domain::ElementRef& ref = m_project_interactor.scene_interactor().object_selection().elements.front();
        Scene::Node& root = m_scene_presenter.scene().root();
        distance = calc_distance(project, ref, root);

        if (m_preset_manager.get_font_prop().per_glyph)
            update_volume(); // change position of projected letters
        //ASSERT(Domain::is_approx(distance.value_or(0.f), (float)value, 1e-3f));
    };
    m_dialog->callbacks().rotation_changed = [this](double value) { rotate(value); };
    m_dialog->callbacks().unlock_rotation = [this](bool check) {
        if (check){
            m_up_limit.reset();
        } else { // Limit direction of the up vector on the model, between side and top surface
            m_up_limit = UP_LIMIT;
        }
    };
    m_dialog->callbacks().set_on_face_camera = [this]() {
        const Scene::Camera& camera = m_scene_presenter.scene().camera();
        Domain::Vec3d wanted_dir = -camera.forward();

        const Domain::Project& project = m_project_interactor.selected_project();
        Biz::Scene::SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
        const Domain::ElementRef& ref = scene_interactor.object_selection().elements.front();

        const Domain::ModelInstance& instance = *project.find_instance_by_id(ref.object_id, ref.instance_id);
        const Domain::ModelVolume& volume = (ref.volume_id != 0) ?
            *project.find_volume_by_id(ref.object_id, ref.volume_id) :
            *project.find_object_by_id(ref.object_id)->volumes.front();
        Domain::Transform3d to_world = instance.get_matrix() * volume.get_matrix();
        const Domain::Transform3d& instance_tr = instance.get_matrix();
        const Domain::Transform3d instance_tr_inv = instance_tr.inverse();
        const Domain::Transform3d& volume_tr_inv = volume.get_matrix().inverse();

        Domain::Vec3d world_position = to_world.translation();
        const Biz::Emboss::TextPresetManager::Preset& preset = m_preset_manager.get_preset();
        Domain::Transform3d new_volume_tr = get_volume_transformation(to_world, wanted_dir, world_position, instance_tr_inv,
            preset.angle, preset.distance, m_up_limit);
        Domain::Transform3d volume_relative = instance_tr * new_volume_tr * volume_tr_inv * instance_tr_inv;
        scene_interactor.transform_selection(volume_relative.matrix());

        if (!m_up_limit.has_value()) { // recalculate angle when not locked
            m_preset_manager.get_preset().angle = calc_rotation(project, ref);
            set_dialog_rotation(*m_dialog, m_preset_manager, m_use_deg);
        }

        if (m_preset_manager.get_font_prop().per_glyph) // change position of the text line
            m_text_lines.create_text_lines(m_text_lines.get_lines().size());

        // update shape when needed
        if (m_preset_manager.get_preset().projection.use_surface ||
            m_preset_manager.get_font_prop().per_glyph)
            update_volume();
    };

    // Presets
    m_dialog->callbacks().preset_selection_changed = [this](int id) {
        m_preset_manager.load_preset(static_cast<size_t>(id));
        if (m_preset_manager.get_font_prop().per_glyph) // new preset contain per_glyph
            m_text_lines.create_text_lines(Biz::Emboss::get_count_lines(m_text));
        update_volume();
    };
    m_dialog->callbacks().save_preset_as = [this]() {
        m_preset_manager.save_preset_as(); 
        m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index()); };
    m_dialog->callbacks().save_preset = [this]() { m_preset_manager.store_presets(); };
    m_dialog->callbacks().rename_preset = [this]() {
        m_preset_manager.rename_preset(); 
        m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index()); };
    m_dialog->callbacks().delete_preset = [this]() { 
        if (m_preset_manager.delete_preset()) {
            m_dialog->set_presets(m_preset_manager.get_presets_names(), m_preset_manager.get_preset_index());
            if (m_preset_manager.get_font_prop().per_glyph) // new preset contain per_glyph
                m_text_lines.create_text_lines(Biz::Emboss::get_count_lines(m_text));
            update_volume();
        }
    };

    m_dialog->callbacks().operation_selection_changed = [this](Domain::ModelVolumeType type) {
        update_volume(UpdateParams{ .volume_type = type });
    };
}

TextGizmo::~TextGizmo() {}

bool TextGizmo::enabled() const { return true; };
Scene::ToolType TextGizmo::type() const { return Scene::ToolType::TextGizmo; }

TextGizmo::~TextGizmo() {}

Yoga::Dialog* TextGizmo::unload_ui_dialog()
{
    return m_dialog.release();
}

void TextGizmo::register_commands(Platform::CommandRegistry& registry)
{
    registry.register_command(
        std::make_unique<Platform::FuncCommand>(
            "Create/Edit text",
            [&]() { add_text_by_view_direction(Domain::ModelVolumeType::MODEL_PART); },
            nullptr,
            Platform::KeyboardShortcut{0, Platform::KeyCode::T}
        )
    );
}

Scene::GizmoActivationState TextGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    using App::Platform::MouseButton;
    using App::Platform::MouseEvent;
    const MouseEvent& mouse_event = ctx.mouse_event();
    if (mouse_event.type() == MouseEvent::Type::ButtonDown
        && mouse_event.button() == MouseButton::Right)
    {
        Domain::ModelVolumeType type = Domain::ModelVolumeType::NEGATIVE_VOLUME;
        if(emboss_text(type, ctx.pick_ray(), ctx.pick_results()))
            return Scene::GizmoActivationState::Active; // create volume at pick ray
    }
    return Scene::GizmoActivationState::Inactive;
}

namespace {
void draw_cross_hair(
    const ImVec2& position,
    ImU32 color = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, .75f)),
    float radius = 12.f,
    int num_segments = 0,
    float thickness = 3.f)
{
    auto draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddCircle(position, radius, color, num_segments, thickness);
    auto dirs = { ImVec2{0, 1}, ImVec2{1, 0}, ImVec2{0, -1}, ImVec2{-1, 0} };
    for (const ImVec2& dir : dirs) {
        ImVec2 start(position.x + dir.x * 0.5 * radius, position.y + dir.y * 0.5 * radius);
        ImVec2 end(position.x + dir.x * 1.5 * radius, position.y + dir.y * 1.5 * radius);
        draw_list->AddLine(start, end, color, thickness);
    }
}

void draw_cross_hair(const Drag& drag) {
    const Domain::Vec2d& p = drag.volume_center;
    ImVec2 position((float)p.x(), (float)p.y());
    ImU32 color = ImGui::GetColorU32(drag.valid ?
        ImVec4(1.f, 1.f, 1.f, .65f) : // transparent white (valid)
        ImVec4(1.f, .3f, .3f, .65f) // transparent redish (invalid)
    );
    draw_cross_hair(position, color);
}

} // namespace

bool TextGizmo::on_drag_start(const Scene::GizmoEventContext& ctx)
{
    for (const Scene::NodePickResult& pick : ctx.pick_results()) {
        if (!pick.node->has_tag_of_type<SceneNodeTag>())
            continue; // ignore staff(node) infront of text volume
        
        auto* tag = pick.node->tag_of_type<SceneNodeTag>();
        // Only last seleceted Volume could be dragged over surface
        if (tag->volume_id != m_last_loaded_volume_id.id)
            return false;

        const Domain::Project& project = m_project_interactor.selected_project();
        const Domain::ModelVolume* volume_ptr = 
            project.find_volume_by_id(tag->object_id, tag->volume_id);
        ASSERT(volume_ptr != nullptr);
        const Domain::ModelVolume& volume = *volume_ptr;
        if (volume.get_object()->volumes.size() == 1)
            return false; // Object is moved by default drag

        // calc mouse offset to volume center
        const Domain::ModelInstance* instance_ptr = 
            project.find_instance_by_id(tag->object_id, tag->instance_id);
        ASSERT(instance_ptr != nullptr);

        m_drag = std::make_unique<Drag>();
        m_drag->to_world = instance_ptr->get_matrix() * volume.get_matrix();
        m_drag->instance = instance_ptr->get_matrix();
        m_drag->instance_inv = instance_ptr->get_matrix().inverse();
        m_drag->volume_inv = volume.get_matrix().inverse();
        Domain::Vec3d volume_center = m_drag->to_world.translation();
        // volume center screen coordinate

        const Scene::Camera& camera = m_scene_presenter.scene().camera();
        Domain::Vec2d center_neg_y = camera.project_to_screen_space(volume_center);
        center_neg_y.y() = camera.viewport().height - center_neg_y.y(); // fix negative direction of y
        m_drag->volume_center = center_neg_y;
        Domain::Vec2d mouse_coor(ctx.screen_mouse_x(), ctx.screen_mouse_y());
        m_drag->volume_offset = m_drag->volume_center - mouse_coor;
        // TODO: Not working ImGui Node
        Scene::FuncImguiRenderNodeComponent::RenderFunc imgui_fn = 
            [this](const Scene::Node& node, const Eigen::AlignedBox<float,2>& box) {
                if (m_drag == nullptr) return;
                draw_cross_hair(*m_drag);
            };
        Scene::Scene& scene = m_scene_presenter.scene();
        Scene::NodeBuilder builder{scene};
        builder
            .set_debug_name("Cross hair for volume center -> 2D")
            //.set_transform(m_drag->to_world)
            .set_tag(CrossHairTag{})
            .set_imgui_func(imgui_fn);
        scene.add_child(builder.build().release());
        return true;
    }
    return false;
}

bool TextGizmo::on_dragging(const Scene::GizmoEventContext& ctx) 
{
    Domain::SquareMatrix4d tr = Domain::SquareMatrix4d::Identity();
    if (m_drag == nullptr)
        return false;

    Domain::Vec2d mouse_coor(ctx.screen_mouse_x(), ctx.screen_mouse_y());
    Domain::Vec2d pick = mouse_coor + m_drag->volume_offset;        
    m_drag->volume_center = pick;
    Scene::NodePickResults pick_results;
    Scene::Ray pick_ray;
    // ignor return value
    m_scene_presenter.scene().pick_at(pick.x(), pick.y(), pick_results, &pick_ray); 

    const Domain::Project& project = m_project_interactor.selected_project();
    const Domain::ModelVolume* embossed_volume_ptr = Biz::Emboss::get_volume(project, m_last_loaded_volume_id);
    if (embossed_volume_ptr == nullptr) {
        // cant find last loaded volume -> end dragging
        m_drag = nullptr;
        return false;
    }
    size_t last_loaded_object_id = embossed_volume_ptr->get_object()->id().id;
    
    for (const Scene::NodePickResult& pick : pick_results) {
        if (!pick.node->has_tag_of_type<SceneNodeTag>())
            continue; // only node tag
        auto* tag = pick.node->tag_of_type<SceneNodeTag>();
        if (tag->volume_id == m_last_loaded_volume_id.id)
            continue; // skip itself

        if (tag->object_id != last_loaded_object_id)
            continue; // another object

        const Domain::ModelVolume* volume_ptr =
            project.find_volume_by_id(tag->object_id, tag->volume_id);
        if (volume_ptr == nullptr)
            continue; // weird
        const Domain::ModelVolume& volume = *volume_ptr;
        if (volume.type() != Domain::ModelVolumeType::MODEL_PART)
            continue; // allowe only the model part

        Domain::Vec3d n = pick.cast.normal;
        Domain::Vec3d p = pick_ray.point_at(pick.cast.distance);
        const Biz::Emboss::TextPresetManager::Preset& preset = m_preset_manager.get_preset();
        Domain::Transform3d new_volume_tr = get_volume_transformation(m_drag->to_world, n, p, 
            m_drag->instance_inv, preset.angle, preset.distance, m_up_limit);
        Domain::Transform3d volume_relative = 
            m_drag->instance * new_volume_tr * m_drag->volume_inv * m_drag->instance_inv;
        Domain::SquareMatrix4d tr = volume_relative.matrix();
        m_drag->last_tr = tr;
        m_project_interactor.scene_interactor().transform_selection(tr, m_drag->memento);
        if (!m_up_limit.has_value()) { // recalculate angle when not locked
            Domain::ElementRef ref(tag->object_id, tag->instance_id, m_last_loaded_volume_id.id);
            m_preset_manager.get_preset().angle = calc_rotation(project, ref);
            set_dialog_rotation(*m_dialog, m_preset_manager, m_use_deg);
        }

        if (m_preset_manager.get_font_prop().per_glyph) // recalculate lines
            m_text_lines.create_text_lines(m_text_lines.get_lines().size());

        m_drag->valid = true;
        return true;
    }

    // pick node not found
    m_project_interactor.scene_interactor()
        .transform_selection(m_drag->last_tr, m_drag->memento);
    m_drag->valid = false;
    return true;
}

void TextGizmo::on_drag_finish(){
    m_project_interactor.scene_interactor()
        .finalize_transform_selection(m_drag->memento, false);
    m_drag = nullptr;
    if (m_preset_manager.get_preset().projection.use_surface ||
        m_preset_manager.get_font_prop().per_glyph)
        update_volume();
}
void TextGizmo::on_drag_cancel(){
    m_project_interactor.scene_interactor()
        .finalize_transform_selection(m_drag->memento, true);
    m_drag = nullptr;
}

void TextGizmo::render_imgui()
{
    if (!ImGui::Begin("Text Gizmo"))
        return ImGui::End();
     
    ImGui::TextColored(
        ImVec4(.1f, .9f, .2f, 1.f),
        "RClick add negative volume \n or object on plate"
    );

    ImVec2 input_size(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 3);
    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput
        | ImGuiInputTextFlags_AutoSelectAll;
    ImGui::InputTextMultiline("##emboss_text_input", &m_text, input_size, flags);

    const Domain::FontList& fonts = m_font_manager.get_fonts();
    auto it_font                  = std::find_if(
        fonts.begin(),
        fonts.end(),
        [&path = m_preset_manager.get_preset().emboss_style.descriptor.path](const Domain::FontDescriptor& fd) {
            return fd.path == path;
        }
    );
    std::string selected = (it_font == fonts.end()) ? std::string("Not selected yet") :
                                                        it_font->name;
    if (ImGui::BeginCombo("Font", selected.c_str())) {
        for (const Domain::FontDescriptor& fd : fonts) {
            const bool is_selected = (it_font == fonts.end()) ? false : &fd == &(*it_font);
            if (ImGui::Selectable(fd.name.c_str(), is_selected)) {
                m_preset_manager.get_preset().emboss_style.descriptor = fd;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // pressets

    if (ImGui::BeginCombo(
            "Pressets",
            m_preset_manager.get_preset().emboss_style.descriptor.name.c_str()
        ))
    {
        const auto& styles = m_preset_manager.get_presets();
        for (const Biz::Emboss::TextPresetManager::Preset& style : styles) {
            const bool is_selected = (&style - &styles.front())
                == m_preset_manager.get_preset_index();
            if (ImGui::Selectable(style.emboss_style.descriptor.name.c_str(), is_selected)) {
                m_preset_manager.load_preset(style);
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
        
    ImGui::InputFloat("size_in_mm", &m_preset_manager.get_font_prop().size_in_mm);
    ImGui::InputDouble("depth", &m_preset_manager.get_preset().projection.depth);
    ImGui::Checkbox("use surface", &m_preset_manager.get_preset().projection.use_surface);
    ImGui::Checkbox("per glyph", &m_preset_manager.get_font_prop().per_glyph);

    ImGui::Separator();
    if (ImGui::Button("Update")) update_volume();
    ImGui::SameLine();
    if (ImGui::Button("Close")) close();

    // Till imgui node not working
    if (m_drag != nullptr)
        draw_cross_hair(*m_drag);

    static double from_surface = 1.3;
    ImGui::Text("from surf %f", from_surface);
    ImGui::SameLine();
    if (ImGui::Button("reCalc")) {
        const Domain::Project& project = m_project_interactor.selected_project();
        const Domain::ElementRef& ref = m_project_interactor.scene_interactor().object_selection().elements.front();
        Scene::Node& root = m_scene_presenter.scene().root();
        from_surface = calc_distance(project, ref, root).value_or(0.f);
    }

    if (ImGui::Button("create line")) {
        m_text_lines.create_text_lines(1);
    }

    ImGui::End();
}

void TextGizmo::on_activated()
{
    if (m_preset_manager.get_presets().empty())
        m_preset_manager.init();

    // m_use_inch = wxGetApp().app_config->get_bool("use_inches");
    m_dialog->update_units(m_use_inch);

    m_dialog->set_rotation_lock(!m_up_limit.has_value()); // !! fix for uninitialized tooltip
    m_dialog->set_rotation_lock(m_up_limit.has_value());
    // Re-load installed fonts
    // NOTE: Version 2.9.2 do it on dialog open, now it is on gizmo activation
    const Domain::FontList& fonts = m_font_manager.get_fonts(); 
    m_dialog->set_fonts(fonts);
    if (m_preset_manager.exist_stored_style())
        m_dialog->set_font(m_preset_manager.get_stored_preset()->emboss_style.descriptor, true);
    m_dialog->set_font(m_preset_manager.get_preset().emboss_style.descriptor, false);

    // Register for scene changes
    Biz::Scene::SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.add_listener<Biz::Scene::ISceneSelectionChangedListener>(this);

    // when text volume is not selected, create new one
    if (Biz::Emboss::get_selected_text_volume(m_project_interactor) == nullptr) {
        // create new text volume
        m_preset_manager.discard_preset_changes();
        // TRN: default text embossed from model
        m_text = _u8L("Embosed text");

        // What shows few miliseconds till new item is created?
        m_dialog->set_enable_all_except_font(true);

        add_text_by_view_direction(Domain::ModelVolumeType::MODEL_PART);
        // after create it is called function on_scene_selection_changed()
        return;
    }

    // set current state of scene
    Domain::SelectionId project_id = m_project_interactor.selected_project_id();
    on_scene_selection_changed(project_id, scene_interactor.object_selection());
}

void TextGizmo::on_deactivated() {
    Biz::Scene::SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.remove_listener<Biz::Scene::ISceneSelectionChangedListener>(this);
}

Scene::ToolType TextGizmo::type() const {
    return Scene::ToolType::Text;
}

namespace {
void set_dialog_rotation(TextDialog& dialog, const Biz::Emboss::TextPresetManager& preset_manager, bool use_deg) {
    bool exist_stored = preset_manager.exist_stored_style();
    const Biz::Emboss::TextPresetManager::Preset& preset = preset_manager.get_preset();
    const Biz::Emboss::TextPresetManager::Preset& preset_ = exist_stored ?
        *preset_manager.get_stored_preset() : preset;

    if (use_deg) {
        double rotation_max = 180.;
        double rotation_step = 0.1;
        double rotation = preset.angle.value_or(0.f) * 180 / M_PI;
        double rotation_ = preset_.angle.value_or(0.f) * 180 / M_PI;
        dialog.set_rotation(rotation_max, rotation_step, rotation, rotation_);
    } else {
        dialog.set_rotation(M_PI, 1e-2, preset.angle.value_or(0.f), preset_.angle.value_or(0.f));
    }
}

void set_dialog_surface_distance(TextDialog& dialog, const Biz::Emboss::TextPresetManager& preset_manager, bool use_inch) {
    bool exist_stored = preset_manager.exist_stored_style();
    const Biz::Emboss::TextPresetManager::Preset& preset = preset_manager.get_preset();
    const Biz::Emboss::TextPresetManager::Preset& preset_ = exist_stored ?
        *preset_manager.get_stored_preset() : preset;

    if (use_inch) {
        double surface_distance = preset.distance.value_or(0.f) * mm_to_inch;
        double surface_distance_ = preset_.distance.value_or(0.f) * mm_to_inch;
        dialog.set_surface_distance(.5, 0.005, surface_distance, surface_distance_);
    } else {
        double surface_distance = preset.distance.value_or(0.f);
        double surface_distance_ = preset_.distance.value_or(0.f);
        dialog.set_surface_distance(2., 0.01, surface_distance, surface_distance_);
    }
}

void activate_preset(TextDialog& dialog, const Biz::Emboss::TextPresetManager& preset_manager,
    double scale_char_gap,
    double scale_line_gap,
    bool use_inch = false,
    bool use_deg = true // otherwise radians
    )
{    
    bool exist_stored = preset_manager.exist_stored_style();
    const Biz::Emboss::TextPresetManager::Preset& preset = preset_manager.get_preset();
    const Biz::Emboss::TextPresetManager::Preset& preset_ = exist_stored ?
        *preset_manager.get_stored_preset() : preset;

    // TODO: solve conversion from font name
    const Domain::EmbossStyle& es = preset.emboss_style;
    const Domain::EmbossStyle& es_ = preset_.emboss_style;
    if (exist_stored)
        dialog.set_font(es_.descriptor, true);
    dialog.set_font(es.descriptor, false);
    
    const Domain::FontProp& prop = es.prop;
    const Domain::FontProp& prop_ = es_.prop;
    if (use_inch) {
        double height = prop.size_in_mm * mm_to_inch;
        double height_default = prop_.size_in_mm * mm_to_inch;
        dialog.set_height(.005, 4., .005, .05, height, height_default);
    } else {
        dialog.set_height(.1, 100., .1, 1, prop.size_in_mm, prop_.size_in_mm);
    }

    const Domain::EmbossProjection& ep = preset.projection;
    const Domain::EmbossProjection& ep_ = preset_.projection;
    if (use_inch) {
        dialog.set_depth(.005, 4., .005, .05, ep.depth * mm_to_inch, ep_.depth * mm_to_inch);
    } else {
        dialog.set_depth(.1, 100., .1, 1, ep.depth, ep_.depth);
    }
    dialog.set_use_surface(ep.use_surface, ep_.use_surface);
    dialog.set_per_glyph(prop.per_glyph, prop_.per_glyph);
    dialog.set_align(prop.align, prop_.align);

    double char_gap_in_mm = prop.char_gap.value_or(0) * scale_char_gap;
    double char_gap_in_mm_ = prop_.char_gap.value_or(0) * scale_char_gap;
    double char_gap_max = use_inch ? .2 /* inch */ : 5. /* mm */;
    double char_gap_step = use_inch ? .005 /* inch */ : .1 /* mm */;
    dialog.set_char_gap(char_gap_max, char_gap_step, char_gap_in_mm, char_gap_in_mm_);

    double line_gap_max = char_gap_max;
    double line_gap_step = char_gap_step;
    double line_gap_in_mm = prop.line_gap.value_or(0) * scale_line_gap;
    double line_gap_in_mm_ = prop_.line_gap.value_or(0) * scale_line_gap;
    dialog.set_line_gap(line_gap_max, line_gap_step, line_gap_in_mm, line_gap_in_mm_);

    double boldness_max = use_inch ? .2 /* inch */ : 5. /* mm */;
    double boldness_step = use_inch ? .005 /* inch */ : .1 /* mm */;
    double boldness_in_mm = prop.boldness.value_or(0) * scale_char_gap;
    double boldness_in_mm_ = prop_.boldness.value_or(0) * scale_char_gap;
    dialog.set_boldness(boldness_max, boldness_step, boldness_in_mm, boldness_in_mm_);

    double skew_ratio_max = 1.;
    double skew_ratio_step = 0.01;
    double skew_ratio = prop.skew.value_or(0.f);
    double skew_ratio_ = prop_.skew.value_or(0.f);
    dialog.set_skew_ratio(skew_ratio_max, skew_ratio_step, skew_ratio, skew_ratio_);

    set_dialog_surface_distance(dialog, preset_manager, use_inch);
    set_dialog_rotation(dialog, preset_manager, use_deg);

    // NOTE: not neccessary to write pressets names every time when volume loads
    dialog.set_presets(preset_manager.get_presets_names(), preset_manager.get_preset_index());
}
} // namespace

void TextGizmo::on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection)
{
    const Domain::Project& project = m_project_interactor.workbench().project(project_id);
    const Domain::ModelVolume* volume_ptr = Biz::Emboss::get_selected_text_volume(project, selection);
    if (volume_ptr == nullptr)
        return close(); // unselection text volume

    const Domain::ModelVolume& volume = *volume_ptr;
    if (m_last_loaded_volume_id == volume.id())
        return; // already loaded
    
    // disable callback till new values are set
    TextDialog::Callbacks temp_callbacks = std::move(m_dialog->callbacks());
    m_dialog->callbacks() = TextDialog::Callbacks{};
    ScopeGuard sg_callbacks([this, &temp_callbacks]() { m_dialog->callbacks() = std::move(temp_callbacks); });

    // load current settings
    const Domain::TextConfiguration& tc = *volume.text_configuration;
    m_text = tc.text;
    const Domain::EmbossStyle& style = tc.style;

    const Domain::ElementRef& ref = selection.elements.front();
    Biz::Emboss::TextPresetManager::Preset preset {
        .emboss_style = style,
        .projection = volume.emboss_shape->projection,
        .angle = calc_rotation(project, ref)
    };

    bool is_part = volume.get_object()->volumes.size() != 1;
    if (!is_part) { // is object
        // Appear after deleting object after adding part
        if (volume.text_configuration->style.prop.per_glyph)
            preset.emboss_style.prop.per_glyph = false;
        if (volume.emboss_shape->projection.use_surface)
            preset.projection.use_surface = false;
    }
    bool use_surface = preset.projection.use_surface;
    m_dialog->show_part_specific_panel(is_part);
    m_dialog->set_enable_surface_distance(is_part && !use_surface);
    m_dialog->set_enable_use_surface(is_part);
    m_dialog->set_enable_per_glyph(is_part);
    if (is_part)
        m_dialog->set_operation(volume.type());

    set_dialog_rotation(*m_dialog, m_preset_manager, m_use_deg);

    const auto& presets = m_preset_manager.get_presets();
    auto preset_it = std::find_if(presets.begin(), presets.end(),
        [&name = style.descriptor.name](const Biz::Emboss::TextPresetManager::Preset& preset) {
            return preset.emboss_style.descriptor.name == name;
        });

    if (preset_it == presets.end()) {
        // unknown preset inside volume, create temporary one
        m_preset_manager.load_preset(preset);
    } else {
        m_preset_manager.load_preset(presets.begin() - preset_it);
        m_preset_manager.get_preset() = preset;
    }

    // Update dialog data
    m_dialog->set_editor(m_text);
    unsigned count_lines = Biz::Emboss::get_count_lines(m_text);
    bool is_multiline = count_lines > 1;
    m_dialog->set_enable_line_gap(is_multiline);
    m_use_deg = true; // or radians
    activate_preset(*m_dialog, m_preset_manager, m_volume_scale.char_gap, m_volume_scale.line_gap, m_use_inch, m_use_deg);   

    calc_scale(project, ref); // volume scale for each axis
    if (is_part) {
        Scene::Node& root = m_scene_presenter.scene().root();
        m_preset_manager.get_preset().distance = calc_distance(project, ref, root);
        set_dialog_surface_distance(*m_dialog, m_preset_manager, m_use_inch);
    }

    if (m_preset_manager.get_font_prop().per_glyph && !m_text_lines.exist_lines())
        m_text_lines.create_text_lines(count_lines);

    m_last_loaded_volume_id = volume.id();
}

namespace {
Domain::Transform3d world_tr(const Domain::Project& project, const Domain::ElementRef& ref) {
    const Domain::ModelInstance& instance = *project.find_instance_by_id(ref.object_id, ref.instance_id);
    const Domain::ModelVolume& volume = (ref.volume_id != 0) ?
        *project.find_volume_by_id(ref.object_id, ref.volume_id) :
        *project.find_object_by_id(ref.object_id)->volumes.front();
    return instance.get_matrix() * volume.get_matrix();
}
} // namespace

// return exist change
bool TextGizmo::calc_scale(const Domain::Project& project, const Domain::ElementRef& ref) {
    Domain::Transform3d to_world = world_tr(project, ref);
    auto to_world_linear = to_world.linear();
    auto calc = [&to_world_linear](const Domain::Vec3d& axe, std::optional<float>& scale) {
        Domain::Vec3d  axe_world = to_world_linear * axe;
        double norm_sq = axe_world.squaredNorm();
        if (Domain::is_approx(norm_sq, 1.)) {
            if (scale.has_value())
                scale.reset();
            else
                return false;
        }
        else {
            scale = sqrt(norm_sq);
        }
        return true;
    };

    bool exist_change = calc(Domain::Vec3d::UnitY(), m_volume_scale.height);
    exist_change |= calc(Domain::Vec3d::UnitX(), m_volume_scale.width);
    exist_change |= calc(Domain::Vec3d::UnitZ(), m_volume_scale.depth);

    auto font_point_to_world = [this](const std::optional<float>& scale)->double {
        const Domain::FontFile& ff = *m_preset_manager.get_font_file_with_cache().font_file; /* not const */
        const Domain::FontProp& fp = m_preset_manager.get_font_prop();
        const Domain::FontFile::Info& font_info = Biz::Emboss::get_font_info(ff, fp);
        double font_point_to_volume_mm = fp.size_in_mm / (double)font_info.unit_per_em;
        double font_point_to_world_mm = font_point_to_volume_mm * scale.value_or(1.f);
        if (m_use_inch)
            return font_point_to_world_mm / 25.4; // * mm_to_inch;
        return font_point_to_world_mm;
    };

    // TODO: solve first initialization and than recaluculate only when exist change
    m_volume_scale.char_gap = font_point_to_world(m_volume_scale.width);
    m_volume_scale.line_gap = font_point_to_world(m_volume_scale.height);
    
    return exist_change;
}

std::optional<float> calc_rotation(const Domain::Project& project, const Domain::ElementRef& ref) {
    Domain::Transform3d to_world = world_tr(project, ref);
    return Biz::Emboss::calc_up(to_world, UP_LIMIT);
}

std::optional<float> calc_distance(const Domain::Project& project, const Domain::ElementRef& ref, Scene::Node& root) {
    Domain::Transform3d world = world_tr(project, ref);
    Domain::Vec3d pos_world = world.translation();
    // Emboss direction in world coordinate(negative Z axis)
    Domain::Vec3d dir_world = world.linear() * Domain::Vec3d::UnitZ() * -1.;
    dir_world.normalize();

    auto ray_cast = [&ref, &root](const Scene::Ray& ray) {
        return Scene::visit_conditional_transform<Scene::RaycastResult>(root,
            [&ray, &ref](Scene::Node& n, Scene::RaycastResult& t) {
                auto* tag = n.tag_of_type<App::Plater::SceneNodeTag>();
                if (tag == nullptr || // Not a scene node tag (object)
                    tag->volume_type != Domain::ModelVolumeType::MODEL_PART ||
                    tag->object_id != ref.object_id || // different object
                    tag->instance_id != ref.instance_id || // different instance
                    tag->volume_id == ref.volume_id || // skip itself
                    !n.has_raycast_component()) // only for sure
                    return false;
                return n.raycast_component()->raycast(n.world_transform().matrix(), ray, t);
            });
        };

    double overlap = 1.;
    Scene::Ray ray_positive{ .origin = pos_world - dir_world * overlap, .direction = dir_world };
    Scene::Ray ray_negative{ .origin = pos_world + dir_world * overlap, .direction = -dir_world };
    auto r_positive = ray_cast(ray_positive);
    auto r_negative = ray_cast(ray_negative);
    for (auto& r : r_positive) r.second.distance = r.second.distance - overlap;
    for (auto& r : r_negative) r.second.distance = overlap - r.second.distance;
    r_positive.insert(r_positive.end(), r_negative.begin(), r_negative.end());
    if (r_positive.empty())
        return {}; // no intersection

    std::sort(r_positive.begin(), r_positive.end(), [](const auto& a, const auto& b) {
        return fabs(a.second.distance) < fabs(b.second.distance);  });

    const auto& closest = r_positive.front();
    double distance = closest.second.distance;
    if (Domain::is_approx(distance, 0., 1e-4))
        return {}; // numerical discrepancy -> lay on surface

    return distance;
}

bool TextGizmo::add_text_by_view_direction(Domain::ModelVolumeType volume_type)
{
    if (!init_create(volume_type))
        return false;    

    // get (pickray + pickresults) from screen center
    Scene::Scene& scene = static_cast<Scene::ISceneProvider&>(m_scene_presenter).scene();
    const Render::Rect& v = scene.camera().viewport();
    Domain::Point logic_center{ v.x + v.width / 2, v.y + v.height / 2 };
    Scene::NodePickResults pick_results;
    Scene::Ray pick_ray;
    scene.pick_at(logic_center.x(), logic_center.y(), pick_results, &pick_ray);

    return emboss_text(volume_type, pick_ray, pick_results);
}

namespace {
bool is_text_empty(std::string_view text) {
    return text.empty() || text.find_first_not_of(" \n\t\r") == std::string::npos; 
}

Biz::Emboss::BaseData create_base_data(
    const std::string& text,
    Domain::ModelVolumeType volume_type,
    Biz::Emboss::TextPresetManager& preset_manager,
    Biz::ProjectInteractor& project_interactor,
    const Biz::Emboss::TextLines& text_lines
) {
    const Biz::Emboss::TextPresetManager::Preset& preset = preset_manager.get_preset();
    Domain::TextConfiguration text_config{ .style = preset.emboss_style, .text = text };
    Domain::SelectionId project_id = project_interactor.selected_project_id();
    Biz::Emboss::FontFileWithCache& font_file = preset_manager.get_font_file_with_cache();
    return Biz::Emboss::BaseData{
        .shape_provider = std::make_unique<Biz::Emboss::TextShapeProvider>(
            std::move(text_config),
            preset.projection,
            text_lines,
            font_file
        ),
        .project_interactor = project_interactor,
        .project_id = project_id,
        .gizmo = static_cast<uint8_t>(App::Scene::ToolType::Text),
        .is_outside = (volume_type == Domain::ModelVolumeType::MODEL_PART),
        .volume_name = "Embossed textik"
    };
}

} // namespace

bool TextGizmo::update_volume(const UpdateParams& update_params) {
    const Domain::ModelVolume* volume_ptr = Biz::Emboss::get_selected_text_volume(m_project_interactor);
    if (volume_ptr == nullptr)
        return false; // no volume selected

    // check that selection did not change without call 'on_scene_selection_changed()'
    const Domain::ModelVolume& volume = *volume_ptr;
    if (m_last_loaded_volume_id != volume.id())
        return false;

    // exist loadeable font file?
    if (!m_preset_manager.get_font_file_with_cache().has_value())
        return false;

    // without text there is nothing to emboss
    if (is_text_empty(m_text)) 
        return false;

    Domain::ModelVolumeType new_type = update_params.volume_type.value_or(volume_ptr->type());
    Biz::Emboss::UpdateVolumeParams params{
        .base = create_base_data(m_text, new_type, m_preset_manager, m_project_interactor, m_text_lines.get_lines()),
        .volume_id = volume.id(),
        .volume_trmat = update_params.volume_transformation,
        .volume_type = update_params.volume_type
    };

    // remove_notification_not_valid_font();
    return start_update_volume(std::move(params), volume);
}

void TextGizmo::close()
{
    m_gizmo_manager.deactivate_current_tool();
}

void TextGizmo::rotate(double absolut_angle) {
    double value = absolut_angle;
    if (m_use_deg)
        value *= M_PI / 180;
    double current = m_preset_manager.get_preset().angle.value_or(0.f);
    if (Domain::is_approx(current, value, 1e-3))
        return; // approx same

    double diff_angle = value - current;
    Domain::Transform3d relative_volume_tr{ Eigen::AngleAxisd(diff_angle, Domain::Vec3d::UnitZ()) };
    transform_selection_relative(relative_volume_tr, m_project_interactor);

    // recalculate current rotation
    const Domain::Project& project = m_project_interactor.selected_project();
    const Domain::ElementRef& el = m_project_interactor.scene_interactor().object_selection().elements.front();
    m_preset_manager.get_preset().angle = calc_rotation(project, el);

    // Is set what was wanted?
    // assert(Domain::is_approx(m_preset_manager.get_preset().angle.value_or(0.f), (float)value, 1e-3f));

    if (m_preset_manager.get_font_prop().per_glyph) // recalculate lines
        m_text_lines.create_text_lines(m_text_lines.get_lines().size());

    // update shape when needed
    if (m_preset_manager.get_preset().projection.use_surface ||
        m_preset_manager.get_font_prop().per_glyph)
        update_volume();
}

bool TextGizmo::init_create(Domain::ModelVolumeType volume_type)
{
    if (volume_type != Domain::ModelVolumeType::MODEL_PART
        && volume_type != Domain::ModelVolumeType::NEGATIVE_VOLUME
        && volume_type != Domain::ModelVolumeType::PARAMETER_MODIFIER)
        return false; // invalid volume type for emboss text

    // if (wxGetApp().obj_list()->has_selected_cut_object()) return false;
    return true;
}

bool TextGizmo::emboss_text(Domain::ModelVolumeType volume_type, const Scene::Ray& ray, const Scene::NodePickResults& results)
{
    Biz::Emboss::CreateVolumeParams params{
        .base = create_base_data(m_text, volume_type, m_preset_manager, m_project_interactor, m_text_lines.get_lines()),
        .volume_type = volume_type
    };
    return Biz::Emboss::start_create_volume(params, ray, results);
}

} // namespace Slic3r::App::Plater

namespace {

// function copied from file: src/slic3r/GUI/SurfaceDrag.cpp
Domain::Transform3d get_volume_transformation(
    Domain::Transform3d world, // from volume
    const Domain::Vec3d& world_dir, // wanted new direction
    const Domain::Vec3d& world_position, // wanted new position
    // Invers transformation of text volume instance
    // Help convert world transformation to instance space 
    const Domain::Transform3d& instance_inv,
    // initial rotation in Z axis
    const std::optional<float>& current_angle,
    const std::optional<float>& current_distance,
    const std::optional<double>& up_limit)
{
    auto world_linear = world.linear();
    // Calculate offset: transformation to wanted position
    {
        // Reset skew of the text Z axis:
        // Project the old Z axis into a new Z axis, which is perpendicular to the old XY plane.
        Domain::Vec3d old_z = world_linear.col(2);
        Domain::Vec3d new_z = world_linear.col(0).cross(world_linear.col(1));
        world_linear.col(2) = new_z * (old_z.dot(new_z) / new_z.squaredNorm());
    }

    Domain::Vec3d       text_z_world = world_linear.col(2); // world_linear * Vec3d::UnitZ()
    auto        z_rotation = Eigen::Quaternion<double, Eigen::DontAlign>::FromTwoVectors(text_z_world, world_dir);
    Domain::Transform3d world_new = z_rotation * world;
    auto        world_new_linear = world_new.linear();

    // Fix direction of up vector to zero initial rotation 
    if (up_limit.has_value()) {
        Domain::Vec3d z_world = world_new_linear.col(2);
        z_world.normalize();
        Domain::Vec3d wanted_up = Biz::Emboss::suggest_up(z_world, *up_limit);

        Domain::Vec3d y_world = world_new_linear.col(1);
        auto  y_rotation = Eigen::Quaternion<double, Eigen::DontAlign>::FromTwoVectors(y_world, wanted_up);

        world_new = y_rotation * world_new;
        world_new_linear = world_new.linear();
    }

    // Edit position from right
    Domain::Transform3d volume_new{ Eigen::Translation<double, 3>(instance_inv * world_position) };
    volume_new.linear() = instance_inv.linear() * world_new_linear;

    // Check that transformation matrix is valid transformation
    assert(volume_new.matrix()(0, 0) == volume_new.matrix()(0, 0)); // Check valid transformation not a NAN
    if (volume_new.matrix()(0, 0) != volume_new.matrix()(0, 0))
        return Domain::Transform3d::Identity();

    // Check that scale in world did not changed
    //assert(!calc_scale(world_linear, world_new_linear, Domain::Vec3d::UnitY()).has_value());
    //assert(!calc_scale(world_linear, world_new_linear, Domain::Vec3d::UnitZ()).has_value());

    // apply move in Z direction and rotation by up vector
    if (up_limit.has_value()) {
        Biz::Emboss::apply_transformation(current_angle, current_distance, volume_new);
    } else {
        // angle is allowed to change
        Biz::Emboss::apply_transformation({}, current_distance, volume_new);
    }
    return volume_new;
}
} // namespace
