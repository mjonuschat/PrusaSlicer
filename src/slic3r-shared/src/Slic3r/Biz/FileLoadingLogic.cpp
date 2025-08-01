#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "tl/expected.hpp"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <Slic3r/App/IDialogManager.hpp>

#include "Slic3r/App/I18N/I18N.hpp"
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
            full_stats.volume += stats.volume
                * std::fabs(trans.matrix().block(0, 0, 3, 3).determinant());
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
            auto copy_volume = [o, max_extruders, &counter, &extruder_counter](ModelVolume* new_v) {
                assert(new_v != nullptr);
                new_v->name = (counter > 1) ? o->name + "_" + std::to_string(counter++) : o->name;
                // ! ysTODO: uncomment, when we will correct get a munber of extruders
                // new_v->volume_settings.overrides.set("extruder", auto_extruder_id(max_extruders, extruder_counter));
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

static void process_mesh(
    TriangleMesh& mesh,
    const std::string& input_file,
    Answer* answer_convert_from_meters,
    Answer* answer_convert_from_imperial_units
)
{
    const TriangleMeshStats& stats = mesh.stats();

    if (looks_like_saved_in_meters(stats)) {
        Answer answer = answer_convert_from_meters ? *answer_convert_from_meters : Answer::Default;
        if (answer_convert_from_meters && *answer_convert_from_meters == Answer::Default) {
            auto& dlg_manager = App::DialogManagerProvider::instance().get();
            dlg_manager.show_rich_yesno_dialog(
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
                [&](bool checked) {
                    if (checked)
                        *answer_convert_from_meters = answer;
                }
            );
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
            auto& dlg_manager = App::DialogManagerProvider::instance().get();
            dlg_manager.show_rich_yesno_dialog(
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
                [&](bool checked) {
                    if (checked)
                        *answer_convert_from_imperial_units = answer;
                }
            );
        }
        if (answer == Answer::Yes) {
            convert_from_imperial_units(mesh);
            // TODO: How to put this information?
            // volume->source.is_converted_from_inches = true;
        }
    }
}

static void remove_objects_with_zero_volume(Model& model, const std::string& file_name)
{
    if (model.objects.size() == 0)
        return;

    int removed = 0;
    for (int i = int(model.objects.size()) - 1; i >= 0; i--)
        if (has_zero_volume(get_object_mesh_stats(model.objects[i]))) {
            model.delete_object(size_t(i));
            removed++;
        }
    if (removed > 0) {
        auto& dlg_manager = App::DialogManagerProvider::instance().get();
        dlg_manager.show_info_dialog(
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

static Loaded3MF load_legacy_project(const std::string& file_path)
{
    Loaded3MF loaded_3mf;
    loaded_3mf.config_containers_data.emplace_back();

    Domain::WipeTowersOnBeds wipe_towers;
    Domain::CustomGCodesOnBeds custom_gcodes;
    std::optional<Semver> version;

    if (!Slic3rLegacy::load_3mf_legacy(
            file_path.c_str(),
            loaded_3mf.config_containers_data.front().config_pack,
            &loaded_3mf.model,
            false,
            loaded_3mf.version,
            wipe_towers,
            custom_gcodes
        ))
        throw Loaded3MFException(
            Read3mfIssue(Read3mfIssueType::legacy_loader_failed, "Loading of legacy 3MF failed.")
        );

    loaded_3mf.filepath_3mf = file_path;
    return loaded_3mf;
}

Loaded3MF load_from_project(const boost::filesystem::path& project_file_path)
{
    Loaded3MF loaded_3mf;
    const std::string project_file_name = project_file_path.string();
    try {
        return load_3mf(project_file_name);
    } catch (const Loaded3MFException& e) {
        if (e.issue.type != Read3mfIssueType::legacy_loader_required)
            throw;
    }
    return load_legacy_project(project_file_name);
}

tl::expected<ReturnData, std::string> read_data_from_file(const boost::filesystem::path& input_file_path)
{
    ReturnData ret = {input_file_path.filename().string()};

    bool result = false;
    if (boost::algorithm::iends_with(input_file_path.string(), ".stl")) {
        auto loaded_mesh = Biz::load_stl(input_file_path.string());
        if (loaded_mesh) {
            Domain::TriangleMesh mesh = loaded_mesh.value();
            ret.mesh                  = mesh;
            return ret;
        }
        return tl::make_unexpected(loaded_mesh.error());
    } else if (boost::algorithm::iends_with(input_file_path.string(), ".3mf")) {
        Loaded3MF loaded_3mf = load_from_project(input_file_path);
        if (loaded_3mf.model.objects.empty()) {
            return tl::make_unexpected(
                fmt::vformat(
                    _u8L("Model from {} couldn't be read because it's empty"),
                    fmt::make_format_args(ret.file_name)
                )
            );
        }

        ret.model = loaded_3mf.model;
        return ret;
    }

    return tl::make_unexpected(_u8L(
        "Unknown file format. Input file must have .stl, .obj, .step/.stp, .svg, .amf(.xml) or extension .3mf(.zip)."
    ));
}

// Loading model from a file, it may be a simple geometry file as STL or OBJ, however it may be a project file as well.
static tl::expected<ReturnData, std::string> read_and_process_file(
    const boost::filesystem::path& input_file_path,
    int tool_count,
    Answer* answer_convert_from_meters         = nullptr,
    Answer* answer_convert_from_imperial_units = nullptr
)
{
    auto data = read_data_from_file(input_file_path);
    if (!data) {
        return data;
    }

    const std::string file_name = input_file_path.filename().string();
    if (data.value().model) {
        Model& model = data.value().model.value();
        remove_objects_with_zero_volume(model, file_name);
    } else if (data.value().mesh) {
        TriangleMesh& mesh = data.value().mesh.value();
        if (has_zero_volume(mesh.stats())) {
            return tl::make_unexpected(
                fmt::vformat(
                    _u8L("Mesh from file {} has zero volume. It will not be loaded."),
                    fmt::make_format_args(file_name)
                )
            );
        }
        process_mesh(mesh, file_name, answer_convert_from_meters, answer_convert_from_imperial_units);
    } else {
        return tl::make_unexpected(_u8L("There is no data for either the mesh or the model."));
    }

    return data;
}

// Loading vector of meshs(from simple geometry file as STL or OBJ) or models(from a 3mf-file) from several files
std::vector<ReturnData> import_files(
    const std::vector<boost::filesystem::path>& input_file_paths,
    int tool_count = 1
)
{
    Answer answer_convert_from_meters         = Answer::Default;
    Answer answer_convert_from_imperial_units = Answer::Default;

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
            &answer_convert_from_meters,
            &answer_convert_from_imperial_units
        );

        if (!data) {
            errors += data.error() + "\n";
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

            // Check if the user actually wants to apply the conversion.
            auto& dlg_manager = App::DialogManagerProvider::instance().get();
            dlg_manager.show_yesno_dialog(
                _u8L("Multi-part object detected"),
                _u8L(
                    "Multiple objects were loaded for a multi-material printer.\n"
                    "Instead of considering them as multiple objects, should I consider\n"
                    "these files to represent a single object having multiple parts?"
                ),
                [&](bool answer) { convert = answer; }
            );

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

    if (!errors.empty()) {
        auto& dlg_manager = App::DialogManagerProvider::instance().get();
        dlg_manager.show_error_dialog(errors, _u8L("Files import") + ":");
    }

    return ret;
}

static Domain::Project convert_to_project(Loaded3MF&& loaded_3mf)
{
    Domain::Project project;
    project.set_metadata(loaded_3mf.metadata);
    project.set_file_name(loaded_3mf.filepath_3mf);
    project.model() = std::move(loaded_3mf.model);

    for (const Loaded3MF::ConfigContainerData& cc_data : loaded_3mf.config_containers_data) {
        project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
        auto& mutable_selected_preset = project.config_containers().back()->mutable_selected_preset();
        mutable_selected_preset = Domain::Preset::SelectedPreset::make(
            cc_data.preset,
            cc_data.config_pack
        );
    }

    if (project.config_containers().empty()) {
        auto cc = std::make_unique<Domain::ConfigContainer>();

        project.config_containers().emplace_back(std::move(cc));
    }
    return project;
}

Domain::Project load_file_as_project(const boost::filesystem::path& project_file_path)
{
    Loaded3MF loaded_3mf = load_from_project(project_file_path);
    if (!loaded_3mf.model.objects.empty()) {
        remove_objects_with_zero_volume(loaded_3mf.model, project_file_path.filename().string());
    };
    return convert_to_project(std::move(loaded_3mf));
}

void import_files_and_add_to_scene(
    const std::vector<boost::filesystem::path>& file_paths,
    int tool_count,
    Scene::SceneInteractor& scene_interactor,
    const Domain::Vec2d& bed_center
)
{
    auto data = Biz::FileLoadingLogic::import_files(file_paths, tool_count);

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
    Scene::SceneInteractor& scene_interactor
)
{
    auto data = Biz::FileLoadingLogic::import_files(file_paths);

    for (Biz::FileLoadingLogic::ReturnData& file_data : data) {
        Domain::BoundingBox3d bbox;
        if (file_data.mesh) {
            auto mesh = file_data.mesh;
            scene_interactor.add_volume_from_mesh(std::move(mesh.value()), volume_type, file_data.file_name);

            bbox = mesh->bounding_box();
        }
        else if (file_data.model) {
            Domain::Model& model = file_data.model.value();
            // Convert objects from the model into separate meshes and add them for selected object
            TriangleMesh mesh;
            for (const ModelObject* object : model.objects) {
                mesh.merge(Biz::Algorithms::ModelObject::mesh(*object));
            }
            scene_interactor.add_volume_from_mesh(std::move(mesh), volume_type, file_data.file_name);
        }
    }
}

} // namespace Slic3r::Biz::FileLoadingLogic
