///|/ Copyright (c) Prusa Research 2021 - 2023 Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/Emboss/EmbossJob.hpp"

#include <stdexcept>
#include <type_traits>
#include <boost/log/trivial.hpp>

#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"

#include "Slic3r/App/I18N/I18N.hpp" // translation
#include "Slic3r/Biz/CGAL/Algorithms/CutSurface.hpp" // use surface cuts

// #include "libslic3r/Format/OBJ.hpp" // load_obj for default mesh
// #include <libslic3r/SLA/ReprojectPointsOnMesh.hpp>
#include "libslic3r/MultipleBeds.hpp"
#include "libslic3r/Utils.hpp"
// #define EXECUTE_UPDATE_ON_MAIN_THREAD // debug execution on main thread

using namespace Slic3r;

using Biz::JThread::StopToken;
using Domain::BoundingBox2crd;
using Domain::BoundingBox3d;
using Domain::BoundingBoxes2crd;
using Domain::EmbossShape;
using Domain::ExPolygonsWithId;
using Domain::ExPolygonsWithIds;
using Domain::ObjectID;
using Domain::TriangleMesh;

using Domain::bounding_box;

// Private implementation for create volume and objects jobs
namespace {
using namespace Slic3r::Biz::Emboss;

// temporary interface for start job
class Job
{
public:
    virtual ~Job()                   = default;
    virtual void process(StopToken&) = 0;

    virtual void finalize() {}
};

/// <summary>
/// Hold neccessary data to create ModelVolume in job
/// Volume is created on the surface of existing volume in object.
/// NOTE: EmbossDataBase::font_file doesn't have to be valid !!!
/// </summary>
struct DataCreateVolume
{
    // Hold data about shape
    BaseData base;

    // define embossed volume type
    Domain::ModelVolumeType volume_type;

    // parent ModelObject index where to create volume
    ObjectID object_id;

    // new created volume transformation
    std::optional<Domain::Transform3d> trmat;
};

// Offset of clossed side to model
constexpr float SAFE_SURFACE_OFFSET = 0.015f; // [in mm]

/// <summary>
/// Create new TextVolume on the surface of ModelObject
/// Should not be stopped
/// NOTE: EmbossDataBase::font_file doesn't have to be valid !!!
/// </summary>
class CreateVolumeJob : public Job
{
    DataCreateVolume m_input;
    TriangleMesh m_result;

public:
    explicit CreateVolumeJob(DataCreateVolume&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

/// <summary>
/// Hold neccessary data to create ModelObject in job
/// Object is placed on bed under screen coor
/// OR to center of scene when it is out of bed shape
/// </summary>
struct DataCreateObject
{
    // Hold data about shape
    BaseData base;

    // Define coordinate on the bed
    Domain::Vec2d bed_coor;

    // additionl rotation around Z axe, given by style settings
    std::optional<float> angle = {};
};

/// <summary>
/// Create new TextObject on the platter
/// Should not be stopped
/// </summary>
class CreateObjectJob : public Job
{
    DataCreateObject m_input;
    TriangleMesh m_result;
    Domain::SquareMatrix4d m_transformation;

public:
    explicit CreateObjectJob(DataCreateObject&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

struct SurfaceVolumeData
{
    // Transformation of volume inside of object
    Domain::Transform3d transform;
    ModelSources sources;
};

/// <summary>
/// Hold neccessary data to create(cut) volume from surface object in job
/// </summary>
struct CreateSurfaceVolumeData : public SurfaceVolumeData
{
    // Hold data about shape
    BaseData base;

    // define embossed volume type
    Domain::ModelVolumeType volume_type;

    // parent ModelObject index where to create volume
    ObjectID object_id;
};

/// <summary>
/// Cut surface from object and create cutted volume
/// Should not be stopped
/// </summary>
class CreateSurfaceVolumeJob : public Job
{
    CreateSurfaceVolumeData m_input;
    TriangleMesh m_result;

public:
    explicit CreateSurfaceVolumeJob(CreateSurfaceVolumeData&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

/// <summary>
/// Hold neccessary data to update embossed text object in job
/// </summary>
struct DataUpdate
{
    BaseData base;

    // unique identifier of volume to change
    Domain::ObjectID volume_id;

    // Used for prevent flooding Undo/Redo stack on slider.
    bool make_snapshot;

    // Transformation of volume after update volume shape
    // NOTE: Add for style change, because it change rotation and distance from surface
    std::optional<Domain::Transform3d> trmat;
};

/// <summary>
/// Hold neccessary data to update embossed text object in job
/// </summary>
struct UpdateSurfaceVolumeData : public DataUpdate, public SurfaceVolumeData
{};

/// <summary>
/// Assert check of inputs data
/// </summary>
bool check(const CreateVolumeParams& input);
bool check(const DataCreateVolume& input, bool is_main_thread = false);
bool check(const DataCreateObject& input);
bool check(const DataUpdate& input, bool is_main_thread = false, bool use_surface = false);
bool check(const SurfaceVolumeData& input);
bool check(const CreateSurfaceVolumeData& input);
bool check(const UpdateSurfaceVolumeData& input);

template <typename Fnc>
static Domain::ExPolygons create_shape(ShapeProvider& input, Fnc was_canceled);

// create sure that emboss object is bigger than source object [in mm]
constexpr float safe_extension = 1.0f;

// <summary>
/// Try to create mesh from text
/// </summary>
/// <param name="input">Text to convert on mesh
/// + Shape of characters + Property of font</param>
/// <param name="font">Font file with cache
/// NOTE: Cache glyphs is changed</param>
/// <param name="was_canceled">To check if process was canceled</param>
/// <returns>Triangle mesh model</returns>
template <typename Fnc>
TriangleMesh try_create_mesh(BaseData& input, const Fnc& was_canceled);
template <typename Fnc>
TriangleMesh create_mesh(BaseData& input, const Fnc& was_canceled);

/// <summary>
/// Create default mesh for embossed text
/// </summary>
/// <returns>Not empty model(index trinagle set - its)</returns>
TriangleMesh create_default_mesh();

/// <summary>
/// Must be called on main thread
/// </summary>
/// <param name="mesh">New mesh data</param>
/// <param name="data">Text configuration, ...</param>
/// <param name="mesh">Transformation of volume</param>
void final_update_volume(
    TriangleMesh&& mesh,
    const DataUpdate& data,
    const Domain::Transform3d* tr = nullptr
);

/// <summary>
/// Add new volume to object
/// </summary>
/// <param name="mesh">triangles of new volume</param>
/// <param name="project_id">Project containing object</param>
/// <param name="object_id">Object where to add volume</param>
/// <param name="type">Type of new volume</param>
/// <param name="trmat">Transformation of volume inside of object</param>
/// <param name="data">Text configuration and New VolumeName</param>
/// <param name="gizmo">Gizmo to open</param>
void create_volume(
    TriangleMesh&& mesh,
    const Domain::SelectionId& project_id,
    const ObjectID& object_id,
    const Domain::ModelVolumeType type,
    const std::optional<Domain::Transform3d>& trmat,
    const BaseData& data,
    int /*GLGizmosManager::EType*/ gizmo
);

/// <summary>
/// Create projection for cut surface from mesh
/// </summary>
/// <param name="tr">Volume transformation in object</param>
/// <param name="shape_scale">Convert shape to milimeters</param>
/// <param name="z_range">Bounding box 3d of model volume for projection ranges</param>
/// <returns>Orthogonal cut_projection</returns>
OrthoProject create_projection_for_cut(
    Domain::Transform3d tr,
    double shape_scale,
    const std::pair<float, float>& z_range
);

/// <summary>
/// Create tranformation for emboss Cutted surface
/// </summary>
/// <param name="is_outside">True .. raise, False .. engrave</param>
/// <param name="emboss">Depth of embossing</param>
/// <param name="tr">Text voliume transformation inside object</param>
/// <param name="cut">Cutted surface from model</param>
/// <returns>Projection</returns>
OrthoProject3d create_emboss_projection(
    bool is_outside,
    float emboss,
    Domain::Transform3d tr,
    Biz::CGAL::Algorithms::SurfaceCut& cut
);

/// <summary>
/// Cut surface into triangle mesh
/// </summary>
/// <param name="base">(can't be const - cache of font)</param>
/// <param name="input2">SurfaceVolume data</param>
/// <param name="was_canceled">Check to interupt execution</param>
/// <returns>Extruded object from cuted surace</returns>
template <typename Fnc>
TriangleMesh cut_surface(/*const*/ BaseData& input1,
                         const SurfaceVolumeData& input2,
                         const Fnc& was_canceled);

/// <summary>
/// Copied triangles from object to be able create mesh for cut surface from
/// </summary>
/// <param name="volumes">Source object volumes for cut surface from</param>
/// <param name="text_volume_id">Source volume id</param>
/// <returns>Source data for cut surface from</returns>
ModelSources create_sources(
    const Domain::ModelVolumePtrs& volumes,
    std::optional<size_t> text_volume_id = {}
);

Domain::ModelVolumePtrs prepare_volumes_to_slice(const Domain::ModelObject& mo);

void create_message(const std::string& message); // only in finalize
bool process(std::exception_ptr& eptr);

class JobException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

auto was_canceled(StopToken& stop)
{
    return [&stop]() {
        return stop.stop_requested();
    };
}

} // namespace

/////////////////
/// Create Volume
CreateVolumeJob::CreateVolumeJob(DataCreateVolume&& input) : m_input(std::move(input))
{
    assert(check(m_input, true));
}

void CreateVolumeJob::process(StopToken& stop)
{
    if (!check(m_input))
        throw std::runtime_error("Bad input data for EmbossCreateVolumeJob.");
    m_result = create_mesh(m_input.base, was_canceled(stop));
}

void CreateVolumeJob::finalize()
{
    if (m_result.its.empty())
        return create_message("Can't create empty volume.");
    create_volume(
        std::move(m_result),
        m_input.base.project_id,
        m_input.object_id,
        m_input.volume_type,
        m_input.trmat,
        m_input.base,
        m_input.base.gizmo
    );
}

/////////////////
/// Create Object
CreateObjectJob::CreateObjectJob(DataCreateObject&& input) : m_input(std::move(input))
{
    assert(check(m_input));
}

void CreateObjectJob::process(StopToken& stop)
{
    if (!check(m_input))
        throw JobException("Bad input data for EmbossCreateObjectJob.");

    // can't create new object with using surface
    EmbossShape& emboss_shape = m_input.base.shape_provider->get_shape();
    if (emboss_shape.projection.use_surface)
        emboss_shape.projection.use_surface = false;

    auto was_canceled = ::was_canceled(stop);
    m_result          = create_mesh(m_input.base, was_canceled);
    if (was_canceled())
        return;

    // check point is on build plate:
    double z = emboss_shape.projection.depth / 2;
    Domain::Vec3d offset(m_input.bed_coor.x(), m_input.bed_coor.y(), z);

    Domain::BoundingBox3d bb3 = m_result.bounding_box();
    offset -= (bb3.max + bb3.min) * .5; // result mesh center

    Domain::Transform3d::TranslationType tt(offset.x(), offset.y(), offset.z());
    Domain::Transform3d transformation(tt);

    // rotate around Z by style settings
    if (m_input.angle.has_value()) {
        std::optional<float> distance; // new object ignore surface distance from style settings
        Biz::Emboss::apply_transformation(m_input.angle, distance, transformation);
    }
    m_transformation = transformation.matrix();
}

void CreateObjectJob::finalize()
{
    if (m_result.empty()) // only for sure
        return create_message("Can't create empty object.");

    const BaseData& base = m_input.base;
    if (base.project_interactor.selected_project_id() != base.project_id)
        m_input.base.project_interactor.select_project(base.project_id);

    Biz::Scene::SceneInteractor& scene_interactor = base.project_interactor.scene_interactor();
    scene_interactor.new_object_from_mesh(std::move(m_result), m_input.base.volume_name);
    // NOTE: function above MUST select just added Object

    const Biz::Scene::ObjectSelection& selection = scene_interactor.object_selection();
    ASSERT(selection.elements.size() == 1);
    const Domain::ElementRef& selected = selection.elements.front();
    Domain::Project& project = base.project_interactor.selected_project();
    Domain::ModelObject* object_ptr = project.find_object_by_id(selected.object_id);
    ASSERT(object_ptr != nullptr);
    ASSERT(object_ptr->volumes.size() == 1);
    Domain::ModelVolume* volume_ptr = object_ptr->volumes.front();
    ASSERT(volume_ptr != nullptr);

    // write emboss data into volume
    m_input.base.shape_provider->write(*volume_ptr);

    // Transform on the wanted place
    scene_interactor.transform_selection(m_transformation);
}

/////////////////
/// Update Volume

/// <summary>
/// Update text shape in existing text volume
/// Predict that there is only one runnig(not canceled) instance of it
/// </summary>
class UpdateJob : public Job
{
    DataUpdate m_input;
    Domain::TriangleMesh m_result;

public:
    // move params to private variable
    explicit UpdateJob(DataUpdate&& input);

    /// <summary>
    /// Create new embossed volume by m_input data and store to m_result
    /// </summary>
    /// <param name="ctl">Control containing cancel flag</param>
    void process(StopToken& stop) override;

    /// <summary>
    /// Update volume - change object_id
    /// </summary>
    /// <param name="canceled">Was process canceled.
    /// NOTE: Be carefull it doesn't care about
    /// time between finished process and started finalize part.</param>
    /// <param name="">unused</param>
    void finalize() override;

    /// <summary>
    /// Update text volume
    /// </summary>
    /// <param name="volume">Volume to be updated</param>
    /// <param name="mesh">New Triangle mesh for volume</param>
    /// <param name="base">Data to write into volume</param>
    static void update_volume(
        Domain::ModelVolume* volume,
        Domain::TriangleMesh&& mesh,
        const Biz::Emboss::BaseData& base
    );
};

UpdateJob::UpdateJob(DataUpdate&& input) : m_input(std::move(input))
{
    assert(check(m_input, true));
}

void UpdateJob::process(StopToken& stop)
{
    if (!check(m_input))
        throw JobException("Bad input data for EmbossUpdateJob.");

    auto was_canceled = ::was_canceled(stop);
    m_result          = ::try_create_mesh(m_input.base, was_canceled);
    if (was_canceled())
        return;
    if (m_result.its.empty())
        throw JobException("Created text volume is empty. Change text or font.");
}

void UpdateJob::finalize()
{
    ::final_update_volume(std::move(m_result), m_input);
}

void UpdateJob::update_volume(
    Domain::ModelVolume* volume,
    Domain::TriangleMesh&& mesh,
    const Biz::Emboss::BaseData& base
)
{
    // check inputs
    bool is_valid_input = volume != nullptr && !mesh.empty() && !base.volume_name.empty();
    assert(is_valid_input);
    if (!is_valid_input)
        return;

    // update volume
    volume->set_mesh(std::move(mesh));
    volume->set_new_unique_id();
    // volume->calculate_convex_hull();

    // write data from base into volume
    base.shape_provider->write(*volume);

    if (volume->name != base.volume_name) {
        volume->name = base.volume_name;
    }

    Domain::ModelObject* object = volume->get_object();
    assert(object != nullptr);
    if (object == nullptr)
        return;

    // GUI_App &app = wxGetApp(); // may be move to input
    // Plater *plater = app.plater();
    // if (plater->printer_technology() == ptSLA)
    // sla::reproject_points_and_holes(object);
    // plater->changed_object(*object);
}

/////////////////
/// Create Surface volume

/// <summary>
/// Update text volume to use surface from object
/// </summary>
class UpdateSurfaceVolumeJob : public Job
{
    UpdateSurfaceVolumeData m_input;
    Domain::TriangleMesh m_result;

public:
    // move params to private variable
    explicit UpdateSurfaceVolumeJob(UpdateSurfaceVolumeData&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

CreateSurfaceVolumeJob::CreateSurfaceVolumeJob(CreateSurfaceVolumeData&& input) :
    m_input(std::move(input))
{
    assert(check(m_input));
}

void CreateSurfaceVolumeJob::process(StopToken& stop)
{
    if (!check(m_input))
        throw JobException("Bad input data for CreateSurfaceVolumeJob.");
    m_result = ::cut_surface(m_input.base, m_input, was_canceled(stop));
}

void CreateSurfaceVolumeJob::finalize()
{
    create_volume(
        std::move(m_result),
        m_input.base.project_id,
        m_input.object_id,
        m_input.volume_type,
        m_input.transform,
        m_input.base,
        m_input.base.gizmo
    );
}

/////////////////
/// Cut Surface
UpdateSurfaceVolumeJob::UpdateSurfaceVolumeJob(UpdateSurfaceVolumeData&& input) :
    m_input(std::move(input))
{
    assert(check(m_input, true));
}

void UpdateSurfaceVolumeJob::process(StopToken& stop)
{
    if (!check(m_input))
        throw JobException("Bad input data for UseSurfaceJob.");
    m_result = cut_surface(m_input.base, m_input, was_canceled(stop));
}

void UpdateSurfaceVolumeJob::finalize()
{
    // when start using surface it is wanted to move text origin on surface of model
    // also when repeteadly move above surface result position should match
    ::final_update_volume(std::move(m_result), m_input, &m_input.transform);
}

namespace {
/// <summary>
/// Check if volume type is possible use for new text volume
/// </summary>
/// <param name="volume_type">Type</param>
/// <returns>True when allowed otherwise false</returns>
bool is_valid(Domain::ModelVolumeType volume_type);

/// <summary>
/// Start job for add new volume to object with given transformation
/// </summary>
/// <param name="object">Define where to add</param>
/// <param name="volume_tr">Wanted volume transformation, when not set will be calculated after creation to be near the object</param>
/// <param name="data">Define what to emboss - shape</param>
/// <param name="volume_type">Type of volume: Part, negative, modifier</param>
/// <returns>Nullptr when job is sucessfully add to worker otherwise return data to be processed different way</returns>
bool start_create_volume_job(
    const Domain::ModelObject& object,
    const std::optional<Domain::Transform3d>& volume_tr,
    BaseData& data,
    Domain::ModelVolumeType volume_type
);

/// <summary>
/// Find volume in selected objects with closest convex hull to screen center.
/// </summary>
/// <param name="selection">Define where to search for closest</param>
/// <param name="screen_center">Canvas center(dependent on camera settings)</param>
/// <param name="objects">Actual objects</param>
/// <param name="closest_center">OUT: coordinate of controid of closest volume</param>
/// <returns>closest volume when exists otherwise nullptr</returns>
// const GLVolume *find_closest(const Selection &selection, const Vec2d &screen_center, const Camera &camera, const ModelObjectPtrs &objects, Vec2d *closest_center);

/// <summary>
/// Start job for add object with text into scene
/// </summary>
/// <param name="input">Contain worker, build shape, gizmo,
/// emboss_data is moved out soo it can't be const</param>
/// <param name="coor">Screen coordinat, where to create new object laying on bed</param>
/// <returns>True when can add job to worker otherwise FALSE</returns>
bool start_create_object_job(CreateVolumeParams& input, const Domain::Vec2d& coor);

/// <summary>
/// Start job to create volume on the surface of object
/// </summary>
/// <param name="input">Variabless needed to create volume</param>
/// <param name="data">Describe what to emboss - shape</param>
/// <param name="screen_coor">Where to add</param>
/// <param name="try_no_coor">True .. try to create volume without screen_coor,
/// False .. </param>
/// <returns>Nullptr when job is sucessfully add to worker otherwise return data to be processed different way</returns>
bool start_create_volume_on_surface_job(
    CreateVolumeParams& input,
    const Domain::Vec2d& screen_coor,
    bool try_no_coor
);

} // namespace

#include <Slic3r/App/Scene/BedNodeTag.hpp>
#include <Slic3r/App/Plater/SceneNodeTag.hpp>

namespace Slic3r::Biz::Emboss {

bool start_create_volume(
    CreateVolumeParams& input,
    const App::Scene::Ray& pick_ray,
    const App::Scene::NodePickResults& picks
)
{
    if (!check(input))
        return false; // bad input data

    const Domain::Project& project = input.base.project_interactor.selected_project();
    for (const App::Scene::NodePickResult& pick : picks) {
        if (pick.node->has_tag_of_type<App::Plater::SceneNodeTag>()) {
            auto* tag = pick.node->tag_of_type<App::Plater::SceneNodeTag>();
            const Domain::ModelVolume* volume = project.find_volume_by_id(tag->object_id, tag->volume_id);
            if (volume == nullptr)
                continue; // no volume under mouse
            // TODO: What to do with Negative volume
            if (volume->type() != Domain::ModelVolumeType::MODEL_PART)
                continue; // skip modifiers + SupportBlock/Enforce

            double UP_LIMIT                       = 0.9;
            Domain::Vec3d pick_point              = pick_ray.point_at(pick.cast.distance);
            Domain::Vec3d pick_normal             = pick.cast.normal;
            const Domain::ModelInstance* instance = project.find_instance_by_id(
                tag->object_id,
                tag->instance_id
            );
            Domain::Transform3d surface_trmat = create_transformation_onto_surface(
                pick_point,
                pick_normal,
                UP_LIMIT
            );
            Domain::Transform3d tr = instance->get_matrix().inverse() * surface_trmat;

            // Create text lines for Per Glyph projection when needed
            const Domain::ModelObject& object = *volume->get_object();
            input.base.shape_provider->create_text_lines(tr, ::prepare_volumes_to_slice(object));

            return start_create_volume_job(object, tr, input.base, input.volume_type);
        }
        if (pick.node->has_tag_of_type<App::Scene::BedNodeTag>()) {
            double d_z = pick_ray.direction.z();
            if (fabs(d_z) - 1e-4 <= 0.) // parallel to Z axis
                break; // no bed under mouse

            Domain::Vec3d z0 = pick_ray.point_at(-pick_ray.origin.z() / d_z);
            Domain::Vec2d bed_coor(z0.x(), z0.y());
            return ::start_create_object_job(input, bed_coor);
        }
    }
    return start_create_volume_without_position(input); // fall back, do not use pick ray
}

bool start_create_volume_without_position(CreateVolumeParams& input)
{
    // need gather: pickray + pickresults


    //return ::start_create_object_job(input, Vec2d(300,300)); // Temporary


    //// select position by camera position and view direction
    // const Selection &selection = input.canvas.get_selection();
    // int object_idx = selection.get_object_idx();

    // Size s = input.canvas.get_canvas_size();
    // Vec2d screen_center(s.get_width() / 2., s.get_height() / 2.);
    // const ModelObjectPtrs &objects = selection.get_model()->objects;

    //// No selected object so create new object
    // if (selection.is_empty() || object_idx < 0 ||
    // static_cast<size_t>(object_idx) >= objects.size())
    // // create Object on center of screen
    // // when ray throw center of screen not hit bed it create object on center of bed
    // return ::start_create_object_job(input, screen_center);

    //// create volume inside of selected object
    // Vec2d coor;
    // const Camera &camera = wxGetApp().plater()->get_camera();
    // input.gl_volume = ::find_closest(selection, screen_center, camera, objects, &coor);
    // if (input.gl_volume == nullptr)
    // return ::start_create_object_job(input, screen_center);
    //
    // bool try_no_coor = false;
    // return ::start_create_volume_on_surface_job(input, coor, try_no_coor);
    return false;
}

bool start_update_volume(UpdateVolumeParams& data)
{
    return false; // TODO: implement update volume job
}

ModelSources create_volume_sources(const Domain::ModelVolume& text_volume)
{
    const Domain::ModelVolumePtrs& volumes = text_volume.get_object()->volumes;
    // no other volume in object
    if (volumes.size() <= 1)
        return {};
    return ::create_sources(volumes, text_volume.id().id);
}

#ifdef EXECUTE_UPDATE_ON_MAIN_THREAD
namespace {
// Run Job on main thread (blocking) - ONLY DEBUG
static inline bool execute_job(std::shared_ptr<Job> j)
{
    struct MyCtl : public Job::Ctl
    {
        void update_status(int st, const std::string& msg = "") override {};

        bool was_canceled() const override
        {
            return false;
        }

        std::future<void> call_on_main_thread(std::function<void()> fn) override
        {
            return std::future<void>{};
        }
    } ctl;

    j->process(ctl);
    wxGetApp().plater()->CallAfter([j]() {
        std::exception_ptr e_ptr = nullptr;
        j->finalize(false, e_ptr);
    });
    return true;
}
} // namespace
#endif

// bool start_update_volume(DataUpdate &&data, const ModelVolume &volume, const Selection &selection, RaycastManager& raycaster)
//{
// assert(data.volume_id == volume.id());
//
// // check cutting from source mesh
// bool &use_surface = data.base->shape.projection.use_surface;
// if (use_surface && volume.is_the_only_one_part())
// use_surface = false;
//
// std::unique_ptr<Job> job = nullptr;
// if (use_surface) {
// // Model to cut surface from.
// SurfaceVolumeData::ModelSources sources = create_volume_sources(volume);
// if (sources.empty())
// return false;
//
// Transform3d volume_tr = volume.get_matrix();
// const std::optional<Transform3d> &fix_3mf = volume.emboss_shape->fix_3mf_tr;
// if (fix_3mf.has_value())
// volume_tr = volume_tr * fix_3mf->inverse();
//
// // when it is new applying of use surface than move origin onto surfaca
// //if (!volume.emboss_shape->projection.use_surface) {
// //    auto offset = calc_surface_offset(selection, raycaster);
// //    if (offset.has_value())
// //        volume_tr *= Eigen::Translation<double, 3>(*offset);
// //}
//
// UpdateSurfaceVolumeData surface_data{std::move(data), {volume_tr, std::move(sources)}};
// job = std::make_unique<UpdateSurfaceVolumeJob>(std::move(surface_data));
// } else {
// job = std::make_unique<UpdateJob>(std::move(data));
// }
//
// #ifndef EXECUTE_UPDATE_ON_MAIN_THREAD
// auto &worker = wxGetApp().plater()->get_ui_job_worker();
// return queue_job(worker, std::move(job));
// #else
// // Run Job on main thread (blocking) - ONLY DEBUG
// return execute_job(std::move(job));
// #endif // EXECUTE_UPDATE_ON_MAIN_THREAD
//}

} // namespace Slic3r::Biz::Emboss

////////////////////////////
/// private namespace implementation
namespace {
bool check(uint8_t /*App::Scene::ToolType*/ gizmo)
{
    return true;
    // assert(gizmo == App::Scene::ToolType::Text || gizmo == App::Scene::ToolType::Svg);
    // return gizmo == App::Scene::ToolType::Text || gizmo == App::Scene::ToolType::Svg;
}

bool check(const ObjectID& object_id)
{
    assert(object_id.valid());
    return object_id.valid();
}

bool check(const BaseData& base)
{
    assert(base.shape_provider != nullptr);
    bool res = base.shape_provider != nullptr;
    res &= check(base.gizmo);
    res &= (base.project_id != Domain::INVALID_ID);
    return res;
}

bool check(const CreateVolumeParams& input)
{
    bool res = is_valid(input.volume_type);
    res &= check(input.base);
    return res;
}

bool check(const DataCreateVolume& input, bool is_main_thread)
{
    bool check_fontfile = false;
    assert(input.base.shape_provider != nullptr);
    bool res = input.base.shape_provider != nullptr;
    res &= is_valid(input.volume_type);
    return res;
}

bool check(const DataCreateObject& input)
{
    bool check_fontfile = false;
    assert(input.base.shape_provider != nullptr);
    bool res = input.base.shape_provider != nullptr;
    return res;
}

bool check(const DataUpdate& input, bool is_main_thread, bool use_surface)
{
    return true; // TODO: check input
    // bool check_fontfile = true;
    // assert(input.base != nullptr);
    // bool res = input.base != nullptr;
    // res &= check(*input.base, check_fontfile, use_surface);
    // assert(input.base->cancel != nullptr);
    // res &= input.base->cancel != nullptr;
    // if (is_main_thread)
    // assert(!input.base->cancel->load());
    // assert(!input.base->shape.projection.use_surface);
    // res &= !input.base->shape.projection.use_surface;
    // return res;
}

bool check(const SurfaceVolumeData& input)
{
    assert(!input.sources.empty());
    bool res = !input.sources.empty();
    return res;
}

bool check(const CreateSurfaceVolumeData& input)
{
    bool res = check((const SurfaceVolumeData&) input);
    res &= check(input.base);
    // res &= check(input.volume_type);
    res &= check(input.object_id);
    assert(!input.sources.empty());
    res &= !input.sources.empty();
    assert(input.base.shape_provider->get_shape().projection.use_surface);
    res &= input.base.shape_provider->get_shape().projection.use_surface;
    return res;
}

bool check(const UpdateSurfaceVolumeData& input)
{
    const DataUpdate& data_update    = input;
    const SurfaceVolumeData& surface = input;
    return check(data_update) && check(surface);
}

template <typename Fnc>
Domain::ExPolygons create_shape(ShapeProvider& input, Fnc was_canceled)
{
    EmbossShape& es = input.get_shape();
    // TODO: improve to use real size of volume
    // ... need world matrix for volume
    // ... printer resolution will be fine too
    return union_with_delta(es, UNION_DELTA, UNION_MAX_ITERATIN);
}

// #define STORE_SAMPLING
#ifdef STORE_SAMPLING
#include "libslic3r/SVG.hpp"
#endif // STORE_SAMPLING

std::vector<BoundingBoxes2crd> create_line_bounds(const ExPolygonsWithIds& shapes, size_t count_lines = 0)
{
    if (count_lines == 0)
        count_lines = get_count_lines(shapes);
    assert(count_lines == get_count_lines(shapes));

    std::vector<BoundingBoxes2crd> result(count_lines);
    size_t text_line_index = 0;
    // s_i .. shape index
    for (const ExPolygonsWithId& shape_id : shapes) {
        const Domain::ExPolygons& shape = shape_id.expoly;
        BoundingBox2crd bb;
        if (!shape.empty()) {
            bb = Biz::Algorithms::ExPolygon::get_extents(shape);
        }
        BoundingBoxes2crd& line_bbs = result[text_line_index];
        line_bbs.push_back(bb);
        if (shape_id.id == ENTER_UNICODE) {
            // skip enters on beginig and tail
            ++text_line_index;
        }
    }
    return result;
}

template <typename Fnc>
TriangleMesh create_mesh_per_glyph(BaseData& input, Fnc was_canceled)
{
    // method use square of coord stored into int64_t
    // static_assert(std::is_same<Point::coord_type, int32_t>());
    const EmbossShape& shape = input.shape_provider->get_shape();
    if (shape.shapes_with_ids.empty())
        return {};

    // Precalculate bounding boxes of glyphs
    // Separate lines of text to vector of Bounds
    assert(get_count_lines(shape.shapes_with_ids) == input.shape_provider->get_text_lines().size());
    size_t count_lines                 = input.shape_provider->get_text_lines().size();
    std::vector<BoundingBoxes2crd> bbs = create_line_bounds(shape.shapes_with_ids, count_lines);

    double depth  = shape.projection.depth / shape.scale;
    auto scale_tr = Eigen::Scaling(shape.scale);

    size_t s_i_offset = 0; // shape index offset(for next lines)
    indexed_triangle_set result;
    for (size_t text_line_index = 0; text_line_index < input.shape_provider->get_text_lines().size();
         ++text_line_index)
    {
        const BoundingBoxes2crd& line_bbs = bbs[text_line_index];
        const TextLine& line              = input.shape_provider->get_text_lines()[text_line_index];
        Biz::Emboss::PolygonPoints samples = sample_slice(line, line_bbs, shape.scale);
        std::vector<double> angles         = calculate_angles(line_bbs, samples, line.polygon);

        for (size_t i = 0; i < line_bbs.size(); ++i) {
            const BoundingBox2crd& letter_bb = line_bbs[i];
            if (!letter_bb.defined)
                continue;

            Domain::Vec2d to_zero_vec = Biz::Algorithms::BoundingBox::center(letter_bb).cast<double>()
                * shape.scale; // [in mm]
            float surface_offset = input.is_outside ?
                -SAFE_SURFACE_OFFSET :
                (-shape.projection.depth + SAFE_SURFACE_OFFSET);

            if (input.from_surface.has_value())
                surface_offset += *input.from_surface;

            Eigen::Translation<double, 3> to_zero(
                -to_zero_vec.x(),
                0.,
                static_cast<double>(surface_offset)
            );

            const double& angle = angles[i];
            Eigen::AngleAxisd rotate(angle + M_PI_2, Domain::Vec3d::UnitY());

            const Biz::Emboss::PolygonPoint& sample = samples[i];
            Domain::Vec2d offset_vec = Biz::Algorithms::Scaling::unscaled<double>(sample.point); // [in mm]
            Eigen::Translation<double, 3> offset_tr(offset_vec.x(), 0., -offset_vec.y());
            Domain::Transform3d tr = offset_tr * rotate * to_zero * scale_tr;

            const Domain::ExPolygons& letter_shape = shape.shapes_with_ids[s_i_offset + i].expoly;
            assert(Biz::Algorithms::ExPolygon::get_extents(letter_shape) == letter_bb);
            auto projectZ = std::make_unique<ProjectZ>(depth);
            ProjectTransform project(std::move(projectZ), tr);
            indexed_triangle_set glyph_its = polygons2model(letter_shape, project);
            Domain::its_merge(result, std::move(glyph_its));

            if (((s_i_offset + i) % 15) && was_canceled())
                return {};
        }
        s_i_offset += line_bbs.size();

#ifdef STORE_SAMPLING
        { // Debug store polygon
            // std::string stl_filepath = "C:/data/temp/line" + std::to_string(text_line_index) + "_model.stl";
            // bool suc = its_write_stl_ascii(stl_filepath.c_str(), "label", result);

            BoundingBox2crd bbox  = get_extents(line.polygon);
            std::string file_path = "C:/data/temp/line"
                + std::to_string(text_line_index)
                + "_letter_position.svg";
            SVG svg(file_path, bbox);
            svg.draw(line.polygon);
            int32_t radius = bbox.size().x() / 300;
            for (size_t i = 0; i < samples.size(); i++) {
                const PolygonPoint& pp = samples[i];
                const Point& p         = pp.point;
                svg.draw(p, "green", radius);
                std::string label = std::string(" ") + tc.text[i];
                svg.draw_text(p, label.c_str(), "black");

                double a      = angles[i];
                double length = 3.0 * radius;
                Point n(length * std::cos(a), length * std::sin(a));
                svg.draw(Slic3r::Line(p - n, p + n), "Lime");
            }
        }
#endif // STORE_SAMPLING
    }
    return Biz::Algorithms::TriangleMesh::construct(std::move(result));
}

template <typename Fnc>
TriangleMesh try_create_mesh(BaseData& input, const Fnc& was_canceled)
{
    if (!input.shape_provider->get_text_lines().empty()) {
        TriangleMesh tm = create_mesh_per_glyph(input, was_canceled);
        if (was_canceled())
            return {};
        if (!tm.empty())
            return tm;
    }

    Domain::ExPolygons shapes = create_shape(*input.shape_provider, was_canceled);
    if (shapes.empty())
        return {};
    if (was_canceled())
        return {};

    // NOTE: SHAPE_SCALE is applied in ProjectZ
    EmbossShape& es = input.shape_provider->get_shape();
    double scale    = es.scale;
    double depth    = es.projection.depth / scale;
    auto projectZ   = std::make_unique<ProjectZ>(depth);
    float offset    = input.is_outside ? -SAFE_SURFACE_OFFSET :
                                         (SAFE_SURFACE_OFFSET - es.projection.depth);
    if (input.from_surface.has_value())
        offset += *input.from_surface;
    Domain::Transform3d tr = Eigen::Translation<double, 3>(0., 0., static_cast<double>(offset))
        * Eigen::Scaling(scale);
    ProjectTransform project(std::move(projectZ), tr);
    if (was_canceled())
        return {};
    return Biz::Algorithms::TriangleMesh::construct(polygons2model(shapes, project));
}

template <typename Fnc>
TriangleMesh create_mesh(BaseData& input, const Fnc& was_canceled)
{
    // It is neccessary to create some shape
    // Emboss text window is opened by creation new emboss text object
    TriangleMesh result = try_create_mesh(input, was_canceled);
    if (was_canceled())
        return {};

    if (result.its.empty()) {
        result = create_default_mesh();
        if (was_canceled())
            return {};
        // only info
        // ctl.call_on_main_thread([]() {
        create_message(
            "It is used default volume for embossed text, try to change text or font to fix it."
        );
        //});
    }

    assert(!result.its.empty());
    return result;
}

TriangleMesh create_default_mesh()
{
    return Biz::Algorithms::TriangleMesh::construct(
        Biz::Algorithms::TriangleMesh::its_make_cube(36., 4., 2.5)
    );
    //// When cant load any font use default object loaded from file
    // std::string  path = Slic3r::resources_dir() + "/data/embossed_text.obj";
    // TriangleMesh triangle_mesh;
    // if (!load_obj(path.c_str(), &triangle_mesh)) {
    // // when can't load mesh use cube
    // return Biz::Algorithms::TriangleMesh::construct(
    // Biz::Algorithms::TriangleMesh::its_make_cube(36., 4., 2.5));
    //}
    // return triangle_mesh;
}

void final_update_volume(TriangleMesh&& mesh, const DataUpdate& data, const Domain::Transform3d* tr)
{
    // for sure that some object will be created
    if (mesh.its.empty())
        return create_message("Empty mesh can't be created.");

    // Plater *plater = wxGetApp().plater();
    //// Check gizmo is still open otherwise job should be canceled
    // assert(plater->canvas3D()->get_gizmos_manager().get_current_type() == GLGizmosManager::Emboss ||
    // plater->canvas3D()->get_gizmos_manager().get_current_type() == GLGizmosManager::Svg);

    // if (data.make_snapshot) {
    // // TRN: This is the title of the action appearing in undo/redo stack.
    // // It is same for Text and SVG.
    // std::string snap_name = _u8L("Emboss attribute change");
    // Plater::TakeSnapshot snapshot(plater, snap_name, UndoRedo::SnapshotType::GizmoAction);
    //}

    Domain::ModelVolume* volume; // = get_model_volume(data.volume_id, plater->model().objects);

    // could appear when user delete edited volume
    if (volume == nullptr)
        return;

    if (data.trmat.has_value()) {
        assert(tr == nullptr);
        tr = &(*data.trmat);
    }

    if (tr != nullptr) {
        volume->set_transformation(*tr);
    } else {
        // apply fix matrix made by store to .3mf
        const std::optional<EmbossShape>& emboss_shape = volume->emboss_shape;
        assert(emboss_shape.has_value());
        if (emboss_shape.has_value() && emboss_shape->fix_3mf_tr.has_value())
            volume->set_transformation(volume->get_matrix() * emboss_shape->fix_3mf_tr->inverse());
    }
    UpdateJob::update_volume(volume, std::move(mesh), data.base);
}

void create_volume(
    TriangleMesh&& mesh,
    const Domain::SelectionId& project_id,
    const ObjectID& object_id,
    const Domain::ModelVolumeType type,
    const std::optional<Domain::Transform3d>& trmat,
    const BaseData& data,
    int /*GLGizmosManager::EType*/ gizmo
)
{
    /* TODO: find way to add volume into not selected project
    auto& project = data.project_interactor.workbench().project(data.project_id);
    /*/
    auto& project = data.project_interactor.selected_project();
    ASSERT(data.project_interactor.selected_project_id() == project_id); // project does not match
    // */

    // create volume
    if (mesh.its.empty())
        return create_message("Can't create empty volume.");

    //// only add volume without addition data
    // scene_interactor.add_volume_from_mesh(std::move(mesh), volume_type, tr.matrix());

    auto obj = project.find_object_by_id(object_id.id);
    if (obj == nullptr)
        return create_message("Bad object to create volume.");

    Domain::Vec3d instance_size; // NOTE: must copy out before adding volume
    if (!trmat.has_value()) {
        // used for align to instance,
        size_t instance_index             = 0; // must exist
        Domain::BoundingBox3d instance_bb = Biz::Algorithms::ModelObject::instance_bounding_box(
            *obj,
            instance_index
        );
        instance_size = Biz::Algorithms::BoundingBox::sizes(instance_bb);
    }

    auto vol  = Biz::Algorithms::ModelObject::add_volume(obj, std::move(mesh), type);
    vol->name = data.volume_name; // copy

    // do not allow model reload from disk
    vol->source.is_from_builtin_objects = true;

    if (trmat.has_value()) {
        vol->set_transformation(*trmat);
    } else {
        assert(!data.shape_provider->get_shape().projection.use_surface);
        // Create transformation for volume near from object(defined by glVolume)
        // Transformation is inspired add generic volumes in ObjectList::load_generic_subobject

        Domain::Vec3d volume_size = Biz::Algorithms::BoundingBox::sizes(vol->mesh().bounding_box());
        // Translate the new modifier to be pickable: move to the left front corner of the instance's bounding box, lift to print bed.
        Domain::Vec3d offset_tr(
            0, // center of instance - Can't suggest width of text before it will be created
            -instance_size.y() / 2 - volume_size.y() / 2, // under
            volume_size.z() / 2 - instance_size.z() / 2
        ); // lay on bed
        // use same instance as for calculation of instance_bounding_box
        Domain::Transform3d
            tr = obj->instances.front()->get_transformation().get_matrix_no_offset().inverse();
        Domain::Transform3d volume_trmat = tr * Eigen::Translation3d(offset_tr);
        vol->set_transformation(volume_trmat);
    }
    data.shape_provider->write(*vol);
    data.project_interactor.scene_interactor().add_volume(vol);
}

OrthoProject create_projection_for_cut(
    Domain::Transform3d tr,
    double shape_scale,
    const std::pair<float, float>& z_range
)
{
    double min_z = z_range.first - safe_extension;
    double max_z = z_range.second + safe_extension;
    assert(min_z < max_z);
    // range between min and max value
    double projection_size                           = max_z - min_z;
    Domain::SquareMatrix3d transformation_for_vector = tr.linear();
    // Projection must be negative value.
    // System of text coordinate
    // X .. from left to right
    // Y .. from bottom to top
    // Z .. from text to eye
    Domain::Vec3d untransformed_direction(0., 0., projection_size);
    Domain::Vec3d project_direction = transformation_for_vector * untransformed_direction;

    // Projection is in direction from far plane
    tr.translate(Domain::Vec3d(0., 0., min_z));
    tr.scale(shape_scale);
    return OrthoProject(tr, project_direction);
}

OrthoProject3d create_emboss_projection(
    bool is_outside,
    float emboss,
    Domain::Transform3d tr,
    Biz::CGAL::Algorithms::SurfaceCut& cut
)
{
    float front_move = (is_outside) ? emboss : SAFE_SURFACE_OFFSET,
          back_move  = -((is_outside) ? SAFE_SURFACE_OFFSET : emboss);
    its_transform(cut, tr.pretranslate(Domain::Vec3d(0., 0., front_move)));
    Domain::Vec3d from_front_to_back(0., 0., back_move - front_move);
    return OrthoProject3d(from_front_to_back);
}

// indexed_triangle_set cut_surface_to_its(const ExPolygons &shapes, const Transform3d& tr,const SurfaceVolumeData::ModelSources &sources, BaseData& input, std::function<bool()> was_canceled) {
// assert(!sources.empty());
// BoundingBox2crd bb = get_extents(shapes);
// double shape_scale = input.shape.scale;
//
// const SurfaceVolumeData::ModelSource *biggest = &sources.front();
//
// size_t biggest_count = 0;
// // convert index from (s)ources to (i)ndexed (t)riangle (s)ets
// std::vector<size_t> s_to_itss(sources.size(), std::numeric_limits<size_t>::max());
// std::vector<indexed_triangle_set>  itss;
// itss.reserve(sources.size());
// for (const SurfaceVolumeData::ModelSource &s : sources) {
// Transform3d mesh_tr_inv       = s.tr.inverse();
// Transform3d cut_projection_tr = mesh_tr_inv * tr;
// std::pair<float, float> z_range{0., 1.};
// OrthoProject cut_projection = create_projection_for_cut(cut_projection_tr, shape_scale, z_range);
// // copy only part of source model
// indexed_triangle_set its = its_cut_AoI(s.mesh->its, bb, cut_projection);
// if (its.indices.empty())
// continue;
// if (biggest_count < its.vertices.size()) {
// biggest_count = its.vertices.size();
// biggest       = &s;
// }
// size_t source_index     = &s - &sources.front();
// size_t its_index        = itss.size();
// s_to_itss[source_index] = its_index;
// itss.emplace_back(std::move(its));
// }
// if (itss.empty())
// return {};
//
// Transform3d tr_inv = biggest->tr.inverse();
// Transform3d cut_projection_tr = tr_inv * tr;
//
// using namespace Algorithms::BoundingBox; // merge,
//
// size_t itss_index = s_to_itss[biggest - &sources.front()];
// BoundingBox3d mesh_bb = bounding_box(itss[itss_index]);
// for (const SurfaceVolumeData::ModelSource &s : sources) {
// itss_index = s_to_itss[&s - &sources.front()];
// if (itss_index == std::numeric_limits<size_t>::max())
// continue;
// if (&s == biggest)
// continue;
//
// Transform3d           tr  = s.tr * tr_inv;
// bool        fix_reflected = true;
// indexed_triangle_set &its = itss[itss_index];
// its_transform(its, tr, fix_reflected);
// BoundingBox3d its_bb = bounding_box(its);
// mesh_bb = merge(mesh_bb, its_bb);
// }
//
// // tr_inv = transformation of mesh inverted
// Transform3d   emboss_tr  = cut_projection_tr.inverse();
// BoundingBox3d mesh_bb_tr = transformed(mesh_bb, emboss_tr);
// std::pair<float, float> z_range{mesh_bb_tr.min.z(), mesh_bb_tr.max.z()};
// OrthoProject cut_projection = create_projection_for_cut(cut_projection_tr, shape_scale, z_range);
// float projection_ratio = (-z_range.first + safe_extension) /
// (z_range.second - z_range.first + 2 * safe_extension);
//
// ExPolygons shapes_data; // is used only when text is reflected to reverse polygon points order
// const ExPolygons *shapes_ptr = &shapes;
// bool is_text_reflected = Slic3r::has_reflection(tr);
// if (is_text_reflected) {
// // revert order of points in expolygons
// // CW --> CCW
// shapes_data = shapes; // copy
// for (ExPolygon &shape : shapes_data) {
// shape.contour.reverse();
// for (Slic3r::Polygon &hole : shape.holes)
// hole.reverse();
// }
// shapes_ptr = &shapes_data;
// }
//
// // Use CGAL to cut surface from triangle mesh
// SurfaceCut cut; // = cut_surface(*shapes_ptr, itss, cut_projection, projection_ratio);
//
// if (is_text_reflected) {
// for (SurfaceCut::Contour &c : cut.contours)
// std::reverse(c.begin(), c.end());
// for (Domain::Index3 &t : cut.indices)
// std::swap(t[0], t[1]);
// }
//
// if (cut.empty()) return {}; // There is no valid surface for text projection.
// if (was_canceled()) return {};
//
// // !! Projection needs to transform cut
// OrthoProject3d projection = create_emboss_projection(input.is_outside, input.shape.projection.depth, emboss_tr, cut);
// return cut2model(cut, projection);
//}

// TriangleMesh cut_per_glyph_surface(BaseData &input1, const SurfaceVolumeData &input2, std::function<bool()> was_canceled)
//{
// // Precalculate bounding boxes of glyphs
// // Separate lines of text to vector of Bounds
// const EmbossShape &es = input1.create_shape();
// if (was_canceled()) return {};
// if (es.shapes_with_ids.empty())
// throw JobException(_u8L("Font doesn't have any shape for given text.").c_str());
//
// assert(get_count_lines(es.shapes_with_ids) == input1.text_lines.size());
// size_t count_lines = input1.text_lines.size();
// std::vector<BoundingBoxes2crd> bbs = create_line_bounds(es.shapes_with_ids, count_lines);
//
// size_t s_i_offset = 0; // shape index offset(for next lines)
// indexed_triangle_set result;
// for (size_t text_line_index = 0; text_line_index < input1.text_lines.size(); ++text_line_index) {
// const BoundingBoxes2crd &line_bbs = bbs[text_line_index];
// const TextLine      &line     = input1.text_lines[text_line_index];
// PolygonPoints        samples  = sample_slice(line, line_bbs, es.scale);
// std::vector<double>  angles   = calculate_angles(line_bbs, samples, line.polygon);
//
// for (size_t i = 0; i < line_bbs.size(); ++i) {
// const BoundingBox2crd &glyph_bb = line_bbs[i];
// if (!glyph_bb.defined)
// continue;
//
// const double &angle = angles[i];
// auto rotate = Eigen::AngleAxisd(angle + M_PI_2, Vec3d::UnitY());
//
// const PolygonPoint &sample = samples[i];
// Vec2d offset_vec = unscale(sample.point); // [in mm]
// auto offset_tr = Eigen::Translation<double, 3>(offset_vec.x(), 0., -offset_vec.y());
//
// ExPolygons glyph_shape = es.shapes_with_ids[s_i_offset + i].expoly;
// assert(get_extents(glyph_shape) == glyph_bb);
//
// Point offset(-glyph_bb.center().x(), 0);
// for (ExPolygon& s: glyph_shape)
// s.translate(offset);
//
// Transform3d modify = offset_tr * rotate;
// Transform3d tr = input2.transform * modify;
// indexed_triangle_set glyph_its = cut_surface_to_its(glyph_shape, tr, input2.sources, input1, was_canceled);
// // move letter in volume on the right position
// its_transform(glyph_its, modify);
//
// // Improve: union instead of merge
// Domain::its_merge(result, std::move(glyph_its));
//
// if (((s_i_offset + i) % 15) && was_canceled())
// return {};
// }
// s_i_offset += line_bbs.size();
// }
//
// if (was_canceled()) return {};
// if (result.empty())
// throw JobException(_u8L("There is no valid surface for text projection.").c_str());
// return Algorithms::TriangleMesh::construct(std::move(result));
//}

// input can't be const - cache of font
template <typename Fnc>
TriangleMesh cut_surface(BaseData& input1, const SurfaceVolumeData& input2, const Fnc& was_canceled)
{
    return {};
    // if (!input1.text_lines.empty())
    // return cut_per_glyph_surface(input1, input2, was_canceled);
    //
    // ExPolygons shapes = create_shape(input1, was_canceled);
    // if (was_canceled()) return {};
    // if (shapes.empty())
    // throw JobException(_u8L("Font doesn't have any shape for given text.").c_str());

    // indexed_triangle_set its = cut_surface_to_its(shapes, input2.transform, input2.sources, input1, was_canceled);
    // if (was_canceled()) return {};
    // if (its.empty())
    // throw JobException(_u8L("There is no valid surface for text projection.").c_str());

    // return Biz::Algorithms::TriangleMesh::construct(std::move(its));
}

ModelSources create_sources(const Domain::ModelVolumePtrs& volumes, std::optional<size_t> text_volume_id)
{
    ModelSources result;
    result.reserve(volumes.size() - 1);
    for (const Domain::ModelVolume* v : volumes) {
        if (text_volume_id.has_value() && v->id().id == *text_volume_id)
            continue;
        // skip modifiers and negative volumes, ...
        if (!v->is_model_part())
            continue;
        const TriangleMesh& tm = v->mesh();
        if (tm.empty())
            continue;
        if (tm.its.empty())
            continue;
        result.push_back({v->mesh_ptr(), v->get_matrix()});
    }
    return result;
}

bool process(std::exception_ptr& eptr)
{
    if (!eptr)
        return false;
    try {
        std::rethrow_exception(eptr);
    } catch (JobException& e) {
        create_message(e.what());
        eptr = nullptr;
    }
    return true;
}

bool finalize(bool canceled, std::exception_ptr& eptr, const BaseData& input)
{
    // doesn't care about exception when process was canceled by user
    if (canceled) {
        eptr = nullptr;
        return false;
    }
    return !process(eptr);
}

bool is_valid(Domain::ModelVolumeType volume_type)
{
    assert(volume_type != Domain::ModelVolumeType::INVALID);
    assert(
        volume_type == Domain::ModelVolumeType::MODEL_PART
        || volume_type == Domain::ModelVolumeType::NEGATIVE_VOLUME
        || volume_type == Domain::ModelVolumeType::PARAMETER_MODIFIER
    );
    if (volume_type == Domain::ModelVolumeType::MODEL_PART
        || volume_type == Domain::ModelVolumeType::NEGATIVE_VOLUME
        || volume_type == Domain::ModelVolumeType::PARAMETER_MODIFIER)
        return true;

    BOOST_LOG_TRIVIAL(error) << "Can't create embossed volume with this type: " << (int) volume_type;
    return false;
}

bool queue_job(std::unique_ptr<Job> job)
{
    std::function<std::unique_ptr<Job>(StopToken, std::unique_ptr<Job>&&)> process =
        [](StopToken stop_token, std::unique_ptr<Job>&& job) -> std::unique_ptr<Job> {
        job->process(stop_token);
        return job;
    };
    std::function<void(std::unique_ptr<Job>&&)> finalize = [](std::unique_ptr<Job>&& job) {
        job->finalize();
    };

    Biz::Platform::PlatformServices::instance()
        .job_manager()
        .create_job("EmbossJob", process, std::move(job))
        .on_result(finalize)
        .start();
    return true;
}

bool start_create_volume_job(
    const Domain::ModelObject& object,
    const std::optional<Domain::Transform3d>& volume_tr,
    BaseData& data,
    Domain::ModelVolumeType volume_type
)
{
    bool& use_surface = data.shape_provider->get_shape().projection.use_surface;
    std::unique_ptr<Job> job;
    if (use_surface) {
        // Model to cut surface from.
        ModelSources sources = create_sources(object.volumes);
        if (sources.empty() || !volume_tr.has_value()) {
            use_surface = false;
        } else {
            SurfaceVolumeData sfvd{*volume_tr, std::move(sources)};
            CreateSurfaceVolumeData surface_data{
                std::move(sfvd),
                std::move(data),
                volume_type,
                object.id()
            };
            job = std::make_unique<CreateSurfaceVolumeJob>(std::move(surface_data));
        }
    }
    if (!use_surface) {
        // create volume
        DataCreateVolume create_volume_data{
            std::move(data),
            volume_type,
            object.id(),
            volume_tr
        };
        job = std::make_unique<CreateVolumeJob>(std::move(create_volume_data));
    }
    return queue_job(std::move(job));
}

// const GLVolume *find_closest(
// const Selection &selection, const Vec2d &screen_center, const Camera &camera, const ModelObjectPtrs &objects, Vec2d *closest_center)
//{
// assert(closest_center != nullptr);
// const GLVolume               *closest = nullptr;
// const Selection::IndicesList &indices = selection.get_volume_idxs();
// assert(!indices.empty()); // no selected volume
// if (indices.empty())
// return closest;
//
// double center_sq_distance = std::numeric_limits<double>::max();
// for (unsigned int id : indices) {
// const GLVolume    *gl_volume = selection.get_volume(id);
// if (const ModelVolume *volume = get_model_volume(*gl_volume, objects);
// volume == nullptr || !volume->is_model_part())
// continue;
// Slic3r::Polygon hull        = CameraUtils::create_hull2d(camera, *gl_volume);
// Vec2d           c           = hull.centroid().cast<double>();
// Vec2d           d           = c - screen_center;
// bool            is_bigger_x = std::fabs(d.x()) > std::fabs(d.y());
// if ((is_bigger_x && d.x() * d.x() > center_sq_distance) ||
// (!is_bigger_x && d.y() * d.y() > center_sq_distance))
// continue;
//
// double distance = d.squaredNorm();
// if (center_sq_distance < distance)
// continue;
// center_sq_distance = distance;
//
// *closest_center = c;
// closest         = gl_volume;
// }
// return closest;
//}

bool start_create_object_job(CreateVolumeParams& input, const Domain::Vec2d& coor)
{
    // create transformation on the coordinate
    DataCreateObject data {
        .base = std::move(input.base), 
        .bed_coor = coor,
        .angle = input.angle
    };    
    auto job = std::make_unique<CreateObjectJob>(std::move(data));
    return queue_job(std::move(job));
}

// for creation volume
Domain::ModelVolumePtrs prepare_volumes_to_slice(const Domain::ModelObject& mo)
{
    const Domain::ModelVolumePtrs& volumes = mo.volumes;
    Domain::ModelVolumePtrs result;
    result.reserve(volumes.size());
    for (Domain::ModelVolume* volume : volumes) {
        // only part could be surface for volumes
        if (!volume->is_model_part())
            continue;

        result.push_back(volume);
    }
    return result;
}

bool start_create_volume_on_surface_job(
    CreateVolumeParams& input,
    const Domain::Vec2d& screen_coor,
    bool try_no_coor
)
{
    return false;
    // auto on_bad_state = [&input, try_no_coor](const ModelObject *object = nullptr) {
    // if (try_no_coor) {
    // // Can't create on coordinate try to create somewhere
    // return start_create_volume_without_position(input);
    // } else {
    // // In centroid of convex hull is not hit with object. e.g. torid
    // // soo create transfomation on border of object

    // // there is no point on surface so no use of surface will be applied
    // if (input.data->shape.projection.use_surface)
    // input.data->shape.projection.use_surface = false;

    // if (object == nullptr)
    // return false;

    // auto gizmo_type = static_cast<int /*GLGizmosManager::EType*/>(input.gizmo);
    // return start_create_volume_job(input.worker, *object, {}, std::move(input.data), input.volume_type, gizmo_type);
    // }
    //};

    // assert(input.gl_volume != nullptr);
    // if (input.gl_volume == nullptr)
    // return on_bad_state();

    // const Model *model = input.canvas.get_model();

    // assert(model != nullptr);
    // if (model == nullptr)
    // return on_bad_state();

    // const ModelObjectPtrs &objects = model->objects;
    // const ModelVolume     *volume  = get_model_volume(*input.gl_volume, objects);
    // assert(volume != nullptr);
    // if (volume == nullptr)
    // return on_bad_state();

    // const ModelInstance *instance = get_model_instance(*input.gl_volume, objects);
    // assert(instance != nullptr);
    // if (instance == nullptr)
    // return on_bad_state();

    // const ModelObject *object = volume->get_object();
    // assert(object != nullptr);
    // if (object == nullptr)
    // return on_bad_state();

    // auto                   cond   = RaycastManager::AllowVolumes({volume->id().id});
    // RaycastManager::Meshes meshes = create_meshes(input.canvas, cond);
    // input.raycaster.actualize(*instance, &cond, &meshes);
    // std::optional<RaycastManager::Hit> hit = ray_from_camera(input.raycaster, screen_coor, input.camera, &cond);

    //// context menu for add text could be open only by right click on an
    //// object. After right click, object is selected and object_idx is set
    //// also hit must exist. But there is options to add text by object list
    // if (!hit.has_value())
    // // When model is broken. It could appear that hit miss the object.
    // // So add part near by in simmilar manner as right panel do
    // return on_bad_state(object);

    //// Create result volume transformation
    // Transform3d surface_trmat = create_transformation_onto_surface(hit->position, hit->normal, UP_LIMIT);
    // apply_transformation(input.angle, input.distance, surface_trmat);
    // Transform3d transform  = instance->get_matrix().inverse() * surface_trmat;
    // auto gizmo_type = static_cast<int /*GLGizmosManager::EType*/>(input.gizmo);

    //// Create text lines for Per Glyph projection when needed
    // input.data->create_text_lines(transform, prepare_volumes_to_slice(*object));
    //
    //// Try to cast ray into scene and find object for add volume
    // return start_create_volume_job(input.worker, *object, transform, std::move(input.data), input.volume_type, gizmo_type);
}

void create_message(const std::string& message)
{
    // TODO: show the message

    // show_error(nullptr, message.c_str());
}

} // namespace
