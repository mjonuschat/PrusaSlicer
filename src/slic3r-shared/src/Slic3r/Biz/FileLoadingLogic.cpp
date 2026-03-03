#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Format/OBJ.hpp"
#include "Slic3r/Biz/Format/STEP.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/ConvexHull.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Biz/Preset/IO/PresetMetadataLegacyLoader.hpp"
#include "Slic3r/Biz/Platform/IAppConfigProvider.hpp"

#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Biz/Scene/BedFactory.hpp"

#include "Slic3r/Directories.hpp"

#include "tl/expected.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "Slic3r/Biz/IMessageDialogProvider.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Config/ConfigLegacy.hpp"

#include <fmt/format.h>

namespace Slic3r::Biz::FileLoadingLogic {

static constexpr const double volume_threshold_inches = 9.0; // 9 = 3*3*3;
static constexpr const double volume_threshold_meters = 0.001; // 0.001 = 0.1*0.1*0.1
static constexpr const double zero_volume             = 0.0000001;

struct ReturnData
{
    std::string file_name;
    std::optional<Domain::TriangleMesh> mesh;
    std::optional<Domain::Model> model;
};

struct FileLoadError
{
    std::string message;
    bool user_cancelled = false;

    static FileLoadError cancelled() { return {"", true}; }
    static FileLoadError error(std::string msg) { return {std::move(msg), false}; }
};

using namespace Domain;

static bool looks_like_imperial_units(const TriangleMeshStats& stats)
{
    return stats.volume < volume_threshold_inches;
}

static bool looks_like_saved_in_meters(const TriangleMeshStats& stats)
{
    return stats.volume < volume_threshold_meters;
}

static bool has_zero_volume(const TriangleMeshStats& stats)
{
    return stats.volume < zero_volume //
        && stats.volume
        > 0.f; // temporary check for the non-legacy project files, where volume is incorrect
}

static void convert_from_imperial_units(TriangleMesh& mesh)
{
    Vec3f versor = 25.4f * Vec3f::Ones();
    mesh.scale(versor);
}

static void convert_from_meters(TriangleMesh& mesh)
{
    Vec3f versor = 1000.f * Vec3f::Ones();
    mesh.scale(versor);
}

// ToDo may be it's not a better place for this function
// It will be used by CutGizmo in the feature
static TriangleMeshStats get_object_mesh_stats(const ModelObject* object)
{
    TriangleMeshStats full_stats;
    full_stats.volume = 0.f;

    // fill full_stats from all objet's meshes
    for (ModelVolume* volume : object->volumes) {
        const TriangleMeshStats& stats = volume->mesh().stats();

        // initialize full_stats (for repaired errors)
        full_stats.open_edges += stats.open_edges;
        full_stats.repaired_errors.merge(stats.repaired_errors);

        // another used satistics value
        if (volume->is_model_part()) {
            Transform3d trans = object->instances.empty() ?
                volume->get_matrix() :
                (volume->get_matrix() * object->instances[0]->get_matrix());
            full_stats.volume +=
                stats.volume * std::fabs(trans.matrix().block(0, 0, 3, 3).determinant());
            full_stats.number_of_parts += stats.number_of_parts;
        }
    }

    return full_stats;
}

// Generate next extruder ID string, in the range of (1, max_extruders).
static int auto_extruder_id(unsigned int max_extruders, unsigned int& cntr)
{
    int out = ++cntr;
    if (cntr == max_extruders)
        cntr = 0;
    return out;
}

static void convert_to_multipart_object(Model& model, unsigned int max_extruders)
{
    assert(model.objects.size() >= 2);
    if (model.objects.size() < 2)
        return;

    Model tmp_model = Model();
    tmp_model.add_object();

    ModelObject* object = tmp_model.objects[0];
    object->input_file  = model.objects.front()->input_file;
    object->name = boost::filesystem::path(model.objects.front()->input_file).stem().string();
    // FIXME copy the config etc?

    unsigned int extruder_counter = 0;
    for (const ModelObject* o : model.objects) {
        for (const ModelVolume* v : o->volumes) {
            // If there are more than one object, put all volumes together
            // Each object may contain any number of volumes and instances
            // The volumes transformations are relative to the object containing them...
            Domain::Transformation trafo_volume = v->get_transformation();
            // Revert the centering operation.
            trafo_volume.set_offset(trafo_volume.get_offset() - o->origin_translation);
            int counter      = 1;
            auto copy_volume = [o, max_extruders, &counter, &extruder_counter](ModelVolume* new_v)
            {
                assert(new_v != nullptr);
                new_v->name = (counter > 1) ? o->name + "_" + std::to_string(counter++) : o->name;
                new_v->volume_settings.overrides.set("extruder", auto_extruder_id(max_extruders, extruder_counter));
                return new_v;
            };
            if (o->instances.empty()) {
                copy_volume(object->add_volume(*v))->set_transformation(trafo_volume);
            } else {
                for (const ModelInstance* i : o->instances)
                    // ...so, transform everything to a common reference system (world)
                    copy_volume(object->add_volume(*v))
                        ->set_transformation(i->get_transformation() * trafo_volume);
            }
        }
    }

    // Note: Don't add default insatnce here.
    // It will be added later during put the model on the bed.

    model.clear_objects();
    model.add_object(*object);
}

enum class Answer
{
    Default,
    Yes,
    No
};

struct StepLoadingMultiple
{
    double linear_precision;
    double angle_precision;
    bool dismiss = false;
};

static void process_mesh(
    TriangleMesh& mesh,
    const std::string& input_file,
    Answer* answer_convert_from_meters,
    Answer* answer_convert_from_imperial_units,
    IMessageDialogProvider* dialog_provider
)
{
    const TriangleMeshStats& stats = mesh.stats();

    if (looks_like_saved_in_meters(stats)) {
        Answer answer = answer_convert_from_meters ? *answer_convert_from_meters : Answer::Default;
        if (answer_convert_from_meters && *answer_convert_from_meters == Answer::Default) {
            if (dialog_provider) {
                dialog_provider->show_rich_yesno_dialog(
                    _u8L("The object is too small"),
                    fmt::vformat(
                        _u8L(
                            "The dimensions of the object from file {} seem to be defined in meters.\n"
                            "The internal unit of PrusaSlicer is a millimeter. Do you want to recalculate the dimensions of the object?"
                        ),
                        fmt::make_format_args(input_file)
                    ) + "\n",

                    _u8L("Apply to all the remaining small objects being loaded."),
                    [&](bool dlg_answer) { answer = dlg_answer ? Answer::Yes : Answer::No; },
                    [&](bool checked)
                    {
                        if (checked)
                            *answer_convert_from_meters = answer;
                    }
                );
            }
        }
        if (answer == Answer::Yes) {
            convert_from_meters(mesh);
            // TODO: How to put this information?
            // volume->source.is_converted_from_meters = true;
        }
    } else if (looks_like_imperial_units(stats)) {
        Answer answer = answer_convert_from_imperial_units ? *answer_convert_from_imperial_units :
                                                             Answer::Default;
        if (answer_convert_from_imperial_units
            && *answer_convert_from_imperial_units == Answer::Default)
        {
            if (dialog_provider) {
                dialog_provider->show_rich_yesno_dialog(
                    _u8L("The object is too small"),
                    fmt::vformat(
                        _u8L(
                            "The dimensions of the object from file {} seem to be defined in inches.\n"
                            "The internal unit of PrusaSlicer is a millimeter. Do you want to recalculate the dimensions of the object?"
                        ),
                        fmt::make_format_args(input_file)
                    ) + "\n",

                    _u8L("Apply to all the remaining small objects being loaded."),
                    [&](bool dlg_answer) { answer = dlg_answer ? Answer::Yes : Answer::No; },
                    [&](bool checked)
                    {
                        if (checked)
                            *answer_convert_from_imperial_units = answer;
                    }
                );
            }
        }
        if (answer == Answer::Yes) {
            convert_from_imperial_units(mesh);
            // TODO: How to put this information?
            // volume->source.is_converted_from_inches = true;
        }
    }
}

static void remove_objects_with_zero_volume(
    Model& model,
    const std::string& file_name,
    IMessageDialogProvider* dialog_provider
)
{
    if (model.objects.size() == 0)
        return;

    int removed = 0;
    for (int i = int(model.objects.size()) - 1; i >= 0; i--)
        if (has_zero_volume(get_object_mesh_stats(model.objects[i]))) {
            model.delete_object(size_t(i));
            removed++;
        }
    if (removed > 0 && dialog_provider) {
        dialog_provider->show_info_dialog(
            fmt::vformat(
                _L_PLURAL_u8(
                    "Some object size from file {} appears to be zero.\n"
                    "This object has been removed from the model",
                    "Some objects size from file {} appears to be zero.\n"
                    "These objects have been removed from the model",
                    removed
                ),
                fmt::make_format_args(file_name)
            ) + "\n",
            _u8L("The size of the object is zero")
        );
    }
}

// The following is only used to convert projects from PS 2.x.
static std::array<int, 2> index2grid_coords(size_t index)
{
    std::array<int, 2> result{0, 0};
    if (index == 0) {
        return result;
    }

    int id = index;
    ++id;
    int a = 1;
    while ((a + 1) * (a + 1) < id)
        ++a;
    id        = id - a * a;
    result[0] = a;
    result[1] = a;
    if (id <= a)
        result[1] = id - 1;
    else
        result[0] = id - a - 1;

    return result;
}

// The following is only used to convert projects from PS 2.x.
static Domain::Vec3d get_bed_translation(int id, double size_x, double size_y, bool legacy_layout)
{
    if (id == 0)
        return Vec3d::Zero();
    int x = 0;
    int y = 0;
    if (legacy_layout)
        x = id;
    else {
        std::array<int, 2> coords{index2grid_coords(id)};
        x = coords[0];
        y = coords[1];
    }

    // As for the m_legacy_layout switch, see comments at definition of bed_gap_relative.
    Vec2d gap    = Vec2d::Ones() * std::min(100., std::hypot(size_x, size_y) * (3. / 10.));
    double gap_x = (legacy_layout ? size_x * (2. / 10.) : gap.x());
    return Vec3d(
        x * (size_x + gap_x),
        y * (size_y + gap.y()), // When using legacy layout, y is zero anyway.
        0.
    );
}

// The following is only used to convert projects from PS 2.x.
// The function fills in bed_offsets in Loaded3MF based on
// instance positions, just like PS 2.x would.
static void infer_bed_positions_and_create_beds(Loaded3MF& loaded_3mf)
{
    if (loaded_3mf.config_containers_data.size() != 1)
        return;
    const auto& cp = loaded_3mf.config_containers_data.front().config_pack;
    std::vector<Vec2d> pts;
    double max_height = 0.;
    if (std::holds_alternative<Domain::ConfigPackFDM>(cp)) {
        pts = std::get<Domain::ConfigPackFDM>(cp)
                  .printer.items.opt("bed_shape")
                  .get<std::vector<Vec2d>>();
        max_height =
            std::get<Domain::ConfigPackFDM>(cp).printer.items.opt("max_print_height").get<double>();
    } else {
        pts = std::get<Domain::ConfigPackSLA>(cp)
                  .sla_printer_settings.items.opt("bed_shape")
                  .get<std::vector<Vec2d>>();
        max_height = std::get<Domain::ConfigPackSLA>(cp)
                         .sla_printer_settings.items.opt("max_print_height")
                         .get<double>();
    }

    using namespace Biz::Algorithms::Bed;

    auto bed = Domain::Bed::from(pts, max_height, std::nullopt, std::nullopt, "", "");
    bed.set_type(detect_bed_type(bed));
    switch (bed.type())
    {
    case Domain::BedType::Circle:
    {
        Algorithms::Geometry::Circled circle = Algorithms::Bed::as_circular_bed(bed);
        bed.set_circle({ circle.center, circle.radius });
        break;
    }
    case Domain::BedType::Convex:
    {
        bed.set_top_bottom_convex_hull_decomposition(Algorithms::Geometry::decompose_convex_polygon_top_bottom(bed.contour()));
        break;
    }
    default: { break; }
    }

    double size_x = bed.contour_aabb_extent().x();
    double size_y = bed.contour_aabb_extent().y();

    std::vector<Domain::BedInstance> bed_instances;
    for (size_t i = 0; i < 9; ++i) {
        bed_instances.emplace_back(bed);
        bed_instances.back().transformation.set_offset(
            get_bed_translation(i, size_x, size_y, false)
        );
    }

    // Find the max index of occupied beds
    // If there is none occupied, leave at least one bed (index 0)
    int max_bed_index = 0;
    for (ModelObject* mo : loaded_3mf.model.objects) {
        for (ModelInstance* mi : mo->instances) {
            const auto bb = Algorithms::ModelObject::instance_bounding_box(*mo, *mi);

            for (int i = 0; i < 9; ++i) {
                if (contains_2d(bed_instances[i], Biz::Algorithms::BoundingBox::to_2d(bb))
                    == BedContainmentState::Inside)
                {
                    max_bed_index = std::max(max_bed_index, i);
                    break;
                }
            }
        }
    }

    for (int i = 0; i <= max_bed_index; ++i) {
        const Vec3d& bed_offset = bed_instances[i].transformation.get_offset();
        loaded_3mf.config_containers_data.front().bed_offsets.emplace_back(
            bed_offset.x(),
            bed_offset.y()
        );
    }
}

using OptionalPresetBundle = std::optional<std::reference_wrapper<const Domain::Preset::Bundle>>;

namespace {
void fix_volume_transformation(ModelVolume& volume) {
    if (!volume.emboss_shape.has_value())
        return;
        
    if (!volume.emboss_shape->legacy_fix_3mf_tr.has_value())
        return; // version without storing fix matrix (can't help)
    
    const Transform3d& fix_tr = *volume.emboss_shape->legacy_fix_3mf_tr;
    volume.set_transformation(volume.get_matrix() * fix_tr.inverse());
    indexed_triangle_set its = volume.mesh_ptr()->its; // copy
    its_transform(its, fix_tr);
    TriangleMesh tm = TriangleMesh(std::move(its));
    volume.set_mesh(std::move(tm));
    // data for fix transformation is not useable anymore
    volume.emboss_shape->legacy_fix_3mf_tr.reset();
}

// For legacy 3mf tranform triangles by fix tr mat
void fix_svg_and_text_transformation(Loaded3MF& loaded_3mf) {
    for (ModelObject* object_ptr : loaded_3mf.model.objects) {
        if (object_ptr == nullptr)
            continue; // should not appear but One never know
        for (ModelVolume* volume_ptr: object_ptr->volumes) {
            if (volume_ptr == nullptr)
                continue; // should not appear but One never know
            fix_volume_transformation(*volume_ptr);
        }
    }
}
}

// The following function is used to load projects from PS 2.x.
static Loaded3MF load_legacy_project(const std::string& file_path, OptionalPresetBundle bundle)
{
    Loaded3MF loaded_3mf;
    loaded_3mf.config_containers_data.emplace_back();

    Domain::CustomGCodesOnBeds custom_gcodes;

    auto& config_pack = loaded_3mf.config_containers_data.back().config_pack;
    LegacyPresetMetadata hw_printer;
    if (!Slic3rLegacy::load_3mf_legacy(
            file_path.c_str(),
            config_pack,
            hw_printer,
            &loaded_3mf.model,
            false,
            loaded_3mf.version,
            loaded_3mf.config_containers_data.back().wipe_towers,
            custom_gcodes
        ))
        throw Loaded3MFException(
            Read3mfIssue(Read3mfIssueType::legacy_loader_failed, "Loading of legacy 3MF failed.")
        );

    // The old slicer did not store HW configuration in the current sense. We must heuristically
    // construct a meaningful SelectedPresetMetadata here by doing something like
    //
    if (bundle.has_value()) {
        auto result =
            Preset::IO::load_legacy_preset_metadata(hw_printer, config_pack, bundle->get());
        if (result.has_value()) {
            auto& preset_metadata = loaded_3mf.config_containers_data.front().preset;
            // FIXME: get relevant metadata from DynamicPrintConfig
            // preset_metadata.printer.name = config_pack;
            preset_metadata = result.value();
            const size_t n_tools = preset_metadata.hw_config.tool_count;
            ASSERT(preset_metadata.tools.size() == n_tools);
            if (std::holds_alternative<ConfigPackFDM>(config_pack)) {
                auto& fdm_config = std::get<ConfigPackFDM>(config_pack);
                if (fdm_config.tool.size() > n_tools) {
                    fdm_config.tool.resize(n_tools);
                    hw_printer.tools.resize(n_tools);
                    fdm_config.resize_tool_parity_items(n_tools, true);
                }
                ASSERT(fdm_config.tool.size() == preset_metadata.tools.size());
                ASSERT(hw_printer.tools.size() == preset_metadata.tools.size());
            }
            // while (preset_metadata.tools.size() < preset_metadata.hw_config.tool_count)
            // preset_metadata.tools.emplace_back();
            ASSERT(preset_metadata.materials.size() == preset_metadata.hw_config.material_slot_count());
            // while (preset_metadata.materials.size() < preset_metadata.hw_config.tool_count)
            // preset_metadata.materials.emplace_back();
        } else
            throw Loaded3MFException(
                Read3mfIssue(Read3mfIssueType::legacy_loader_failed, result.error())
            );
    }
    //
    // This will provide enough info to create a complete Preset further down the road.

    // Old project did not store bed positions explicitly.
    if (loaded_3mf.version && *loaded_3mf.version < Semver(3, 0, 0)) {
        infer_bed_positions_and_create_beds(loaded_3mf);
    
        // Old project contain baked tranformation matrix of the volumes.
        fix_svg_and_text_transformation(loaded_3mf);
    }


    loaded_3mf.filepath_3mf = file_path;
    return loaded_3mf;
}

Loaded3MF
load_from_project(const boost::filesystem::path& project_file_path, OptionalPresetBundle bundle)
{
    const std::string project_file_name = project_file_path.string();
    try {
        return load_3mf(project_file_name);
    } catch (const Loaded3MFException& e) {
        if (e.issue.type != Read3mfIssueType::legacy_loader_required)
            throw;
    }
    return load_legacy_project(project_file_name, bundle);
}

static tl::expected<ReturnData, FileLoadError> read_data_from_file(
    const boost::filesystem::path& input_file_path,
    IMessageDialogProvider* dialog_provider,
    StepLoadingMultiple* step_loading_multiple
)
{
    ReturnData ret = {input_file_path.filename().string()};
    const bool is_stl = boost::algorithm::iends_with(input_file_path.string(), ".stl");
    const bool is_3mf = boost::algorithm::iends_with(input_file_path.string(), ".3mf");
    const bool is_obj = boost::algorithm::iends_with(input_file_path.string(), ".obj");
    const bool is_step = boost::algorithm::iends_with(input_file_path.string(), ".step") || boost::algorithm::iends_with(input_file_path.string(), ".stp");

    if (is_stl || is_obj) {
        auto loaded_mesh = is_stl ? Biz::load_stl(input_file_path.string()) : Biz::load_obj(input_file_path.string());
        if (loaded_mesh) {
            Domain::TriangleMesh mesh = loaded_mesh.value();
            ret.mesh                  = mesh;
            return ret;
        }
        return tl::make_unexpected(FileLoadError::error(loaded_mesh.error()));
    } else if (is_3mf) {
        try {
            Loaded3MF loaded_3mf = load_from_project(input_file_path, std::nullopt);
            if (loaded_3mf.model.objects.empty()) {
                return tl::make_unexpected(FileLoadError::error(
                    fmt::vformat(
                        _u8L("Model from {} couldn't be read because it's empty"),
                        fmt::make_format_args(ret.file_name)
                    )
                ));
            }

            ret.model = loaded_3mf.model;
            return ret;
        } catch (const Loaded3MFException& e) {
            return tl::make_unexpected(FileLoadError::error(
                fmt::vformat(
                    _u8L("Failed to load model from 3MF file {}. {}"),
                    fmt::make_format_args(ret.file_name, e.issue.msg)
                )
            ));
        }
    } else if (is_step) {
        Biz::Platform::IAppConfigProvider& app_config = Platform::PlatformServices::instance().app_config_provider();
        double linear_precision = app_config.get_step_linear_precision();
        double angle_precision = app_config.get_step_angle_precision();
        bool show_dialog = app_config.get_show_step_import_parameters();

        // Check if we should skip the dialog because "Apply to all" was clicked on a previous file
        if (step_loading_multiple && step_loading_multiple->dismiss) {
            linear_precision = step_loading_multiple->linear_precision;
            angle_precision = step_loading_multiple->angle_precision;
            show_dialog = false;
        }

        if (show_dialog) {
            auto dialog_result = dialog_provider->show_load_step_dialog(
                input_file_path.filename().string(),
                linear_precision,
                angle_precision,
                step_loading_multiple != nullptr  // Show "Apply to all" button only when loading multiple files
            );
            if (!dialog_result)
            {
                // User cancelled the dialog
                return tl::make_unexpected(FileLoadError::cancelled());
            }

            linear_precision = dialog_result->linear_precision;
            angle_precision = dialog_result->angle_precision;
            if (dialog_result->do_not_show_again) {
                app_config.set_show_step_import_parameters(false);
            }
            app_config.set_step_linear_precision(linear_precision);
            app_config.set_step_angle_precision(angle_precision);

            // If "Apply to all" was clicked, store the values and suppress dialog for remaining files
            if (dialog_result->apply_to_all && step_loading_multiple) {
                step_loading_multiple->linear_precision = linear_precision;
                step_loading_multiple->angle_precision = angle_precision;
                step_loading_multiple->dismiss = true;
            }
        }
        auto out = load_step(input_file_path.string(), std::make_pair(linear_precision, angle_precision));
        if (out) {
            ret.model = std::move(out.value());
            return ret;
        } else {
            return tl::make_unexpected(FileLoadError::error(out.error()));
        }
    }

    return tl::make_unexpected(FileLoadError::error(_u8L(
        "Unknown file format. Input file must have .stl, .obj, .step/.stp, .svg, .amf(.xml) or extension .3mf(.zip)."
    )));
}

// Loading model from a file, it may be a simple geometry file as STL or OBJ, however it may be a project file as well.
static tl::expected<ReturnData, FileLoadError> read_and_process_file(
    const boost::filesystem::path& input_file_path,
    int tool_count,
    IMessageDialogProvider* dialog_provider,
    Answer* answer_convert_from_meters         = nullptr,
    Answer* answer_convert_from_imperial_units = nullptr,
    StepLoadingMultiple* step_loading_multiple = nullptr
)
{
    auto data = read_data_from_file(input_file_path, dialog_provider, step_loading_multiple);
    if (!data) {
        return data;
    }

    const std::string file_name = input_file_path.filename().string();
    if (data.value().model) {
        Model& model = data.value().model.value();
        remove_objects_with_zero_volume(model, file_name, dialog_provider);
    } else if (data.value().mesh) {
        TriangleMesh& mesh = data.value().mesh.value();
        if (has_zero_volume(mesh.stats())) {
            return tl::make_unexpected(FileLoadError::error(
                fmt::vformat(
                    _u8L("Mesh from file {} has zero volume. It will not be loaded."),
                    fmt::make_format_args(file_name)
                )
            ));
        }
        process_mesh(
            mesh,
            file_name,
            answer_convert_from_meters,
            answer_convert_from_imperial_units,
            dialog_provider
        );
    } else {
        return tl::make_unexpected(FileLoadError::error(_u8L("There is no data for either the mesh or the model.")));
    }

    return data;
}

// Loading vector of meshs(from simple geometry file as STL or OBJ) or models(from a 3mf-file) from several files
static std::vector<ReturnData> import_files(
    const std::vector<boost::filesystem::path>& input_file_paths,
    IMessageDialogProvider* dialog_provider,
    int tool_count = 1
)
{
    Answer answer_convert_from_meters         = Answer::Default;
    Answer answer_convert_from_imperial_units = Answer::Default;
    StepLoadingMultiple step_loading_multiple;

    std::vector<ReturnData> ret;
    std::string errors;

    Domain::Model* extra_model = nullptr;
    {
        // We are offering the option to convert loaded meshes into multi-part objects only
        // if all meshes are loaded from non-project files and
        // the files are intended for multi-tool printers.
        bool can_convert_to_multipart = input_file_paths.size() > 1 && tool_count > 1;
        for (const auto& path : input_file_paths) {
            if (path.extension().string() == "3mf") {
                // To avoid mishmash we don't allow to any changes,
                // if we load some 3mf during multiple files import
                can_convert_to_multipart = false;
            }
        }

        // To enable converting loaded meshes into a multi-part object,
        // we need a model containing separate objects based on those meshes.
        if (can_convert_to_multipart)
            extra_model = new Domain::Model();
    }

    for (const auto& path : input_file_paths) {
        auto data = read_and_process_file(
            path,
            tool_count,
            dialog_provider,
            &answer_convert_from_meters,
            &answer_convert_from_imperial_units,
            input_file_paths.size() > 1 ? &step_loading_multiple : nullptr
        );

        if (!data) {
            // Only accumulate actual errors, skip if user cancelled
            if (!data.error().user_cancelled) {
                errors += data.error().message + "\n";
            }
        } else {
            if (extra_model && data.value().mesh) {
                ModelObject* new_object = extra_model->add_object();
                new_object->name        = path.filename().string();
                new_object->input_file  = path.string();
                Algorithms::ModelObject::add_volume(new_object, data.value().mesh.value());
            } else {
                ret.emplace_back(data.value());
            }
        }
    }

    if (extra_model) {
        bool convert = false;
        if (extra_model->objects.size() > 1) {
            extra_model->add_default_instances();

            if (dialog_provider) {
                // Check if the user actually wants to apply the conversion.
                dialog_provider->show_yesno_dialog(
                    _u8L("Multi-part object detected"),
                    _u8L(
                        "Multiple objects were loaded for a multi-material printer.\n"
                        "Instead of considering them as multiple objects, should I consider\n"
                        "these files to represent a single object having multiple parts?"
                    ),
                    [&](bool answer) { convert = answer; }
                );
            }

            if (convert) {
                // Perform a conversion and return processed model
                convert_to_multipart_object(*extra_model, tool_count);
                ReturnData data;
                data.model = *extra_model;
                ret.emplace_back(data);
            }
        }

        if (!convert) {
            // Otherwise we need to return the separate meshes
            for (const ModelObject* object : extra_model->objects) {
                ReturnData data;
                data.file_name = object->name;
                data.mesh      = object->volumes.front()->mesh();
                ret.emplace_back(data);
            }
        }
    }

    if (!errors.empty() && dialog_provider) {
        dialog_provider->show_error_dialog(errors, _u8L("Files import") + ":");
    }

    return ret;
}

static Domain::Project
convert_to_project(Loaded3MF&& loaded_3mf, IMessageDialogProvider* dialog_provider)
{
    Domain::Project project;
    if (loaded_3mf.config_containers_data.front().preset.hw_config.id.empty()) {
        // If we are here, then legacy project was loaded.
        // Implementation of the config loading is not completed jet, so we can't create a correct project
        if (dialog_provider) {
            dialog_provider->show_info_dialog(
                _u8L("It's not possible to load legacy 3mf temporary."),
                _u8L("Load legacy 3mf file.")
            );
        }
        return project;
    }

    project.set_metadata(loaded_3mf.metadata);
    project.set_file_name(boost::filesystem::path(loaded_3mf.filepath_3mf).stem().string());
    project.model() = std::move(loaded_3mf.model);

    for (const Loaded3MF::ConfigContainerData& cc_data : loaded_3mf.config_containers_data) {
        project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
        ConfigContainer& cc           = *project.config_containers().back().get();
        auto& mutable_selected_preset = cc.mutable_selected_preset();
        mutable_selected_preset =
            Domain::Preset::SelectedPreset::make(cc_data.preset, cc_data.config_pack);

        // Always add one bed instance.
        cc.set_bed(Scene::get_or_create_bed(project.bed_container(), cc, resources_dir()));
        cc.add_bed_instance();

        for (size_t bed_idx = 0; bed_idx < cc_data.bed_offsets.size(); ++bed_idx) {
            const Vec2d& bed_offset = cc_data.bed_offsets[bed_idx];
            if (bed_idx != 0)
                cc.add_bed_instance();
            cc.bed_instances().back().get()->transformation.set_offset(
                Domain::Vec3d(bed_offset.x(), bed_offset.y(), 0.)
            );
            const auto wipe_tower_it{cc_data.wipe_towers.find(bed_idx)};
            if (wipe_tower_it != cc_data.wipe_towers.end()) {
                cc.bed_instances().back()->wipe_tower = cc_data.wipe_towers.at(bed_idx);
            }
        }
    }

    return project;
}

Domain::Project load_file_as_project(
    const boost::filesystem::path& project_file_path,
    const Domain::Preset::Bundle& bundle,
    IMessageDialogProvider* dialog_provider
)
{
    Loaded3MF loaded_3mf = load_from_project(project_file_path, bundle);
    if (!loaded_3mf.model.objects.empty()) {
        remove_objects_with_zero_volume(
            loaded_3mf.model,
            project_file_path.filename().string(),
            dialog_provider
        );
    };
    return convert_to_project(std::move(loaded_3mf), dialog_provider);
}

void import_files_and_add_to_scene(
    const std::vector<boost::filesystem::path>& file_paths,
    int tool_count,
    Scene::SceneInteractor& scene_interactor,
    const Domain::Vec2d& bed_center,
    IMessageDialogProvider* dialog_provider
)
{
    auto data = Biz::FileLoadingLogic::import_files(file_paths, dialog_provider, tool_count);

    for (Biz::FileLoadingLogic::ReturnData& file_data : data) {
        Domain::BoundingBox3d bbox;
        if (file_data.mesh) {
            auto mesh = file_data.mesh;
            scene_interactor.new_object_from_mesh(std::move(mesh.value()), file_data.file_name);

            bbox = mesh->bounding_box();
        } else if (file_data.model) {
            Domain::Model& model = file_data.model.value();
            if (model.objects.size() == 1 && model.objects.front()->instances.empty()) {
                Domain::ModelObject* multi_part_object = model.objects.front();
                // add a default instance and center object around origin
                Biz::Algorithms::ModelObject::center_around_origin(*multi_part_object);
                multi_part_object->add_instance();
                bbox = Biz::Algorithms::ModelObject::raw_bounding_box(*multi_part_object);
            }
            scene_interactor.add_new_objects(model.objects);
        }

        if (bbox.defined) {
            Transform3d xform = Transform3d::Identity();
            using namespace Biz::Algorithms::BoundingBox;
            xform.translate(-center(bbox));
            xform.translate(Vec3d(0., 0., sizes(bbox).z() / 2.));
            xform.translate(Vec3d{bed_center.x(), bed_center.y(), 0});
            scene_interactor.transform_selection(xform.matrix());
        }
    }
}

/**
 * Load meshes from multiple source files and add them into selected object
 */
void import_volumes_into_selected_object(
    const std::vector<boost::filesystem::path>& file_paths,
    const Domain::ModelVolumeType& volume_type,
    Scene::SceneInteractor& scene_interactor,
    IMessageDialogProvider* dialog_provider
)
{
    auto data = Biz::FileLoadingLogic::import_files(file_paths, dialog_provider);

    for (Biz::FileLoadingLogic::ReturnData& file_data : data) {
        Domain::BoundingBox3d bbox;
        if (file_data.mesh) {
            auto mesh = file_data.mesh;
            scene_interactor
                .add_volume_from_mesh(std::move(mesh.value()), volume_type, file_data.file_name);

            bbox = mesh->bounding_box();
        } else if (file_data.model) {
            Domain::Model& model = file_data.model.value();
            // Convert objects from the model into separate meshes and add them for selected object
            TriangleMesh mesh;
            for (const ModelObject* object : model.objects) {
                mesh.merge(Biz::Algorithms::ModelObject::mesh(*object));
            }
            scene_interactor
                .add_volume_from_mesh(std::move(mesh), volume_type, file_data.file_name);
        }
    }
}

tl::expected<Domain::Model, std::string>
read_model_from_file(const std::string& input_file, IMessageDialogProvider* dialog_provider)
{
    const boost::filesystem::path input_file_path = boost::filesystem::path(input_file);
    tl::expected<ReturnData, FileLoadError> processed_file =
        read_and_process_file(input_file_path, 1, dialog_provider);
    if (!processed_file) {
        const FileLoadError& error = processed_file.error();
        // Convert FileLoadError back to string for the public API
        // If user cancelled, provide a meaningful message
        if (error.user_cancelled) {
            return tl::make_unexpected(_u8L("File loading cancelled by user"));
        }
        return tl::make_unexpected(error.message);
    }

    Domain::Model model;
    if (processed_file.value().model.has_value()) {
        model = std::move(processed_file.value().model.value());
        model.add_default_instances();
    } else if (processed_file.value().mesh.has_value()) {
        ModelObject* new_object = model.add_object();
        new_object->name        = input_file_path.filename().string();
        new_object->input_file  = input_file_path.string();
        Algorithms::ModelObject::add_volume(new_object, processed_file.value().mesh.value());
        model.add_default_instances();
    } else {
        return tl::make_unexpected(_u8L("There is no data for either the mesh or the model."));
    }

    return model;
}

bool is_project_file(const std::string& input_file)
{
    return boost::algorithm::iends_with(input_file, ".3mf")
        || boost::algorithm::iends_with(input_file, ".zip");
}

} // namespace Slic3r::Biz::FileLoadingLogic
