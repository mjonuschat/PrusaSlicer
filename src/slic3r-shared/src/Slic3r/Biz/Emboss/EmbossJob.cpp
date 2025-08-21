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

#include <Slic3r/App/Scene/BedNodeTag.hpp>
#include <Slic3r/App/Plater/SceneNodeTag.hpp>
#include "Slic3r/App/I18N/I18N.hpp" // translation
#include "Slic3r/Biz/CGAL/Algorithms/CutSurface.hpp" // use surface cuts

// #include "libslic3r/Format/OBJ.hpp" // load_obj for default mesh
// #include <libslic3r/SLA/ReprojectPointsOnMesh.hpp>
#include "libslic3r/MultipleBeds.hpp"
#include "libslic3r/Utils.hpp"

using namespace Slic3r;
namespace {
using namespace Slic3r::Biz::Emboss;

bool check(const CreateVolumeParams& input);
bool check(const UpdateVolumeParams& input, bool is_main_thread = false, bool use_surface = false);

/// <summary>
/// Start job for add new volume to object with given transformation
/// </summary>
/// <param name="object">Define where to add</param>
/// <param name="volume_tr">Wanted volume transformation, when not set will be calculated after creation to be near the object</param>
/// <param name="data">Define what to emboss - shape</param>
/// <param name="volume_type">Type of volume: Part, negative, modifier</param>
/// <returns>Nullptr when job is sucessfully add to worker otherwise return data to be processed different way</returns>
bool start_create_volume_job(const Domain::ModelObject& object, const std::optional<Domain::Transform3d>& volume_tr, BaseData& data, Domain::ModelVolumeType volume_type);

/// <summary>
/// Start job for add object with text into scene
/// </summary>
/// <param name="input">Contain worker, build shape, gizmo,
/// emboss_data is moved out soo it can't be const</param>
/// <param name="coor">Screen coordinat, where to create new object laying on bed</param>
/// <returns>True when can add job to worker otherwise FALSE</returns>
bool start_create_object_job(CreateVolumeParams& input, const Domain::Vec2d& coor);

Domain::ModelVolumePtrs prepare_volumes_to_slice(const Domain::ModelObject& mo);

using namespace Slic3r::Biz::Emboss;
using Biz::JThread::StopToken;

// temporary interface for start job
class Job {
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
    Domain::ObjectID object_id;

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
    Domain::TriangleMesh m_result;

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
    Domain::TriangleMesh m_result;
    Domain::Transformation m_transformation;

public:
    explicit CreateObjectJob(DataCreateObject&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

/// <summary>
/// Triangle sources for cut surface from volume
/// used only with SurfaceVolumeData
/// </summary>
struct ModelSource
{
    // source volumes
    std::shared_ptr<const Domain::TriangleMesh> mesh;
    // Transformation of volume inside of object
    Domain::Transform3d tr;
};

using ModelSources = std::vector<ModelSource>;

/// <summary>
/// Copied triangles from object to be able create mesh for cut surface from
/// </summary>
/// <param name="volume">Define embossed volume</param>
/// <returns>Source data for cut surface from</returns>
ModelSources create_volume_sources(const Domain::ModelVolume& volume);

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
    Domain::ObjectID object_id;
};

/// <summary>
/// Cut surface from object and create cutted volume
/// Should not be stopped
/// </summary>
class CreateSurfaceVolumeJob : public Job
{
    CreateSurfaceVolumeData m_input;
    Domain::TriangleMesh m_result;

public:
    explicit CreateSurfaceVolumeJob(CreateSurfaceVolumeData&& input);
    void process(StopToken& stop) override;
    void finalize() override;
};

/// <summary>
/// Hold neccessary data to update embossed text object in job
/// </summary>
struct UpdateSurfaceVolumeData : public UpdateVolumeParams, public SurfaceVolumeData{};

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

/// <summary>
/// Update text shape in existing text volume
/// Predict that there is only one runnig(not canceled) instance of it
/// </summary>
class UpdateJob : public Job
{
    UpdateVolumeParams m_input;
    Domain::TriangleMesh m_result;

public:
    // move params to private variable
    explicit UpdateJob(UpdateVolumeParams&& input);

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
    static void update_volume(Domain::ModelVolume& volume, Domain::TriangleMesh&& mesh, const Biz::Emboss::BaseData& base);
};

bool queue_job(std::unique_ptr<Job> job);

} // namespace

namespace Slic3r::Biz::Emboss {
bool start_create_volume(CreateVolumeParams& input, const App::Scene::Ray& pick_ray, const App::Scene::NodePickResults& picks)
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

            double UP_LIMIT           = 0.9;
            Domain::Vec3d pick_point  = pick_ray.point_at(pick.cast.distance);
            Domain::Vec3d pick_normal = pick.cast.normal;
            const Domain::ModelInstance* instance = project.find_instance_by_id(tag->object_id, tag->instance_id);
            Domain::Transform3d surface_trmat = create_transformation_onto_surface(pick_point, pick_normal, UP_LIMIT);
            Domain::Transform3d tr = instance->get_matrix().inverse() * surface_trmat;

            // Create text lines for Per Glyph projection when needed
            const Domain::ModelObject& object = *volume->get_object();
            input.base.shape_provider->create_text_lines(tr, ::prepare_volumes_to_slice(object));

            return ::start_create_volume_job(object, tr, input.base, input.volume_type);
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
    return ::start_create_object_job(input, Domain::Vec2d(0, 0)); // fall back, do not use pick ray
}

bool start_update_volume(UpdateVolumeParams&& data, const Domain::ModelVolume& volume)
{
    // check cutting from source mesh
    bool& use_surface = data.base.shape_provider->get_projection().use_surface;
    if (use_surface && volume.is_the_only_one_part())
        use_surface = false;

    if (!use_surface) {
        // Without surface
        return queue_job(std::make_unique<UpdateJob>(std::move(data)));
    }

    // use surface

    // Model to cut surface from.
    ModelSources sources = create_volume_sources(volume);
    if (sources.empty())
        return false;

    Domain::Transform3d volume_tr                     = volume.get_matrix();
    const std::optional<Domain::Transform3d>& fix_3mf = volume.emboss_shape->fix_3mf_tr;
    if (fix_3mf.has_value())
        volume_tr = volume_tr * fix_3mf->inverse();

    // when it is new applying of use surface than move origin onto surfaca
    // if (!volume.emboss_shape->projection.use_surface) {
    // auto offset = calc_surface_offset(selection, raycaster);
    // if (offset.has_value())
    // volume_tr *= Eigen::Translation<double, 3>(*offset);
    //}

    UpdateSurfaceVolumeData surface_data{std::move(data), {volume_tr, std::move(sources)}};
    return queue_job(std::make_unique<UpdateSurfaceVolumeJob>(std::move(surface_data)));
}
} // namespace Slic3r::Biz::Emboss


// Private implementation for create volume and objects jobs
namespace {
/// <summary>
/// Assert check of inputs data
/// </summary>
bool check(const DataCreateVolume& input, bool is_main_thread = false);
bool check(const DataCreateObject& input);
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
Domain::TriangleMesh try_create_mesh(BaseData& input, const Fnc& was_canceled);
template <typename Fnc>
Domain::TriangleMesh create_mesh(BaseData& input, const Fnc& was_canceled);

/// <summary>
/// Create default mesh for embossed text
/// </summary>
/// <returns>Not empty model(index trinagle set - its)</returns>
Domain::TriangleMesh create_default_mesh();

/// <summary>
/// Must be called on main thread
/// </summary>
/// <param name="mesh">New mesh data</param>
/// <param name="data">Text configuration, ...,
/// Note: NOT CONST -> contain Project interactor to change on the poroject
/// containing updated volume</param>
void final_update_volume(Domain::TriangleMesh&& mesh, UpdateVolumeParams& data);

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
void
create_volume(Domain::TriangleMesh&& mesh, const Domain::SelectionId& project_id, const Domain::ObjectID& object_id, const Domain::ModelVolumeType type, const std::optional<Domain::Transform3d>& trmat, const BaseData& data, int /*GLGizmosManager::EType*/ gizmo);

/// <summary>
/// Create projection for cut surface from mesh
/// </summary>
/// <param name="tr">Volume transformation in object</param>
/// <param name="shape_scale">Convert shape to milimeters</param>
/// <param name="z_range">Bounding box 3d of model volume for projection ranges</param>
/// <returns>Orthogonal cut_projection</returns>
OrthoProject create_projection_for_cut(Domain::Transform3d tr, double shape_scale, const std::pair<float, float>& z_range);

/// <summary>
/// Create tranformation for emboss Cutted surface
/// </summary>
/// <param name="is_outside">True .. raise, False .. engrave</param>
/// <param name="emboss">Depth of embossing</param>
/// <param name="tr">Text voliume transformation inside object</param>
/// <param name="cut">Cutted surface from model</param>
/// <returns>Projection</returns>
OrthoProject3d
create_emboss_projection(bool is_outside, float emboss, Domain::Transform3d tr, Biz::CGAL::Algorithms::SurfaceCut& cut);

/// <summary>
/// Cut surface into triangle mesh
/// </summary>
/// <param name="base">(can't be const - cache of font)</param>
/// <param name="input2">SurfaceVolume data</param>
/// <param name="was_canceled">Check to interupt execution</param>
/// <returns>Extruded object from cuted surace</returns>
template <typename Fnc>
Domain::TriangleMesh cut_surface(/*const*/ BaseData& input1, const SurfaceVolumeData& input2, const Fnc& was_canceled
);

/// <summary>
/// Copied triangles from object to be able create mesh for cut surface from
/// </summary>
/// <param name="volumes">Source object volumes for cut surface from</param>
/// <param name="text_volume_id">Source volume id</param>
/// <returns>Source data for cut surface from</returns>
ModelSources create_sources(const Domain::ModelVolumePtrs& volumes, std::optional<size_t> text_volume_id = {});

void create_message(const std::string& message); // only in finalize
bool process(std::exception_ptr& eptr);

class JobException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

auto was_canceled(StopToken& stop)
{
    return [&stop]() { return stop.stop_requested(); };
}

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
    Domain::EmbossShape& emboss_shape = m_input.base.shape_provider->get_shape();
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
    m_transformation = Domain::Transformation(transformation);
}

void CreateObjectJob::finalize()
{
    if (m_result.empty()) // only for sure
        return create_message("Can't create empty object.");

    const BaseData& base = m_input.base;
    if (base.project_interactor.selected_project_id() != base.project_id)
        m_input.base.project_interactor.select_project(base.project_id);

    Biz::Scene::SceneInteractor& scene_interactor = base.project_interactor.scene_interactor();

    Biz::Scene::SceneInteractor::UpdateObjectFn update_object = [&](Domain::ModelObject& object)
    {
        object.name                 = m_input.base.volume_name;
        Domain::ModelVolume& volume = *object.volumes.front();
        volume.name                 = m_input.base.volume_name;
        m_input.base.shape_provider->write(volume);

        Domain::ModelInstance& instance = *object.instances.front();
        instance.set_transformation(m_transformation);
    };
    scene_interactor.new_object_from_mesh(std::move(m_result), base.project_id, update_object);
}

/////////////////
/// Update Volume

UpdateJob::UpdateJob(UpdateVolumeParams&& input) : m_input(std::move(input))
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

void UpdateJob::update_volume(Domain::ModelVolume& volume, Domain::TriangleMesh&& mesh, const Biz::Emboss::BaseData& base)
{
    // check inputs
    bool is_valid_input = !mesh.empty();
    assert(is_valid_input);
    if (!is_valid_input)
        return;

    // write data from base into volume
    base.shape_provider->write(volume);
    if (volume.name != base.volume_name && !base.volume_name.empty()) {
        volume.name = base.volume_name;
    }

    Domain::ModelObject* object = volume.get_object();
    assert(object != nullptr);
    if (object == nullptr)
        return;

    size_t object_id = object->id().id;
    Domain::ElementRef ref(object_id, 0, volume.id().id);

    Biz::Scene::SceneInteractor::RefMeshes meshes;
    using Biz::Algorithms::TriangleMesh::construct;
    meshes.emplace_back(ref, std::move(mesh));

    Biz::Scene::SceneInteractor& scene_interactor = base.project_interactor.scene_interactor();
    scene_interactor.change_volume_meshes(std::move(meshes));
}

/////////////////
/// Create Surface volume

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
    ::final_update_volume(std::move(m_result), m_input);
}

////////////////////////////
/// private namespace implementation

ModelSources create_volume_sources(const Domain::ModelVolume& text_volume)
{
    const Domain::ModelVolumePtrs& volumes = text_volume.get_object()->volumes;
    // no other volume in object
    if (volumes.size() <= 1)
        return {};
    return ::create_sources(volumes, text_volume.id().id);
}

bool check(uint8_t /*App::Scene::ToolType*/ gizmo)
{
    return true;
    // assert(gizmo == App::Scene::ToolType::Text || gizmo == App::Scene::ToolType::Svg);
    // return gizmo == App::Scene::ToolType::Text || gizmo == App::Scene::ToolType::Svg;
}

bool check(const Domain::ObjectID& object_id)
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

bool check(Domain::ModelVolumeType volume_type)
{
    assert(volume_type != Domain::ModelVolumeType::INVALID);
    assert(volume_type == Domain::ModelVolumeType::MODEL_PART || volume_type == Domain::ModelVolumeType::NEGATIVE_VOLUME || volume_type == Domain::ModelVolumeType::PARAMETER_MODIFIER);
    if (volume_type == Domain::ModelVolumeType::MODEL_PART || volume_type == Domain::ModelVolumeType::NEGATIVE_VOLUME || volume_type == Domain::ModelVolumeType::PARAMETER_MODIFIER)
        return true;

    BOOST_LOG_TRIVIAL(error) << "Can't create embossed volume with this type: " << (int)volume_type;
    return false;
}

bool check(const CreateVolumeParams& input)
{
    bool res = check(input.volume_type);
    res &= check(input.base);
    return res;
}

bool check(const DataCreateVolume& input, bool is_main_thread)
{
    bool check_fontfile = false;
    assert(input.base.shape_provider != nullptr);
    bool res = input.base.shape_provider != nullptr;
    res &= check(input.volume_type);
    return res;
}

bool check(const DataCreateObject& input)
{
    bool check_fontfile = false;
    assert(input.base.shape_provider != nullptr);
    bool res = input.base.shape_provider != nullptr;
    return res;
}

bool check(const UpdateVolumeParams& input, bool is_main_thread, bool use_surface)
{
    bool res = check(input.base);
    assert(!input.volume_id.invalid());
    res &= !input.volume_id.invalid();
    return res;
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
    const UpdateVolumeParams& data_update = input;
    const SurfaceVolumeData& surface      = input;
    return check(data_update) && check(surface);
}

template <typename Fnc>
Domain::ExPolygons create_shape(ShapeProvider& input, Fnc was_canceled)
{
    Domain::EmbossShape& es = input.get_shape();
    // TODO: improve to use real size of volume
    // ... need world matrix for volume
    // ... printer resolution will be fine too
    return union_with_delta(es, UNION_DELTA, UNION_MAX_ITERATIN);
}

// #define STORE_SAMPLING
#ifdef STORE_SAMPLING
#include "libslic3r/SVG.hpp"
#endif // STORE_SAMPLING

std::vector<Domain::BoundingBoxes2crd> create_line_bounds(const Domain::ExPolygonsWithIds& shapes, size_t count_lines = 0)
{
    if (count_lines == 0)
        count_lines = get_count_lines(shapes);
    assert(count_lines == get_count_lines(shapes));

    std::vector<Domain::BoundingBoxes2crd> result(count_lines);
    size_t text_line_index = 0;
    // s_i .. shape index
    for (const Domain::ExPolygonsWithId& shape_id : shapes) {
        const Domain::ExPolygons& shape = shape_id.expoly;
        Domain::BoundingBox2crd bb;
        if (!shape.empty()) {
            bb = Biz::Algorithms::ExPolygon::get_extents(shape);
        }
        Domain::BoundingBoxes2crd& line_bbs = result[text_line_index];
        line_bbs.push_back(bb);
        if (shape_id.id == ENTER_UNICODE) {
            // skip enters on beginig and tail
            ++text_line_index;
        }
    }
    return result;
}

template <typename Fnc>
Domain::TriangleMesh create_mesh_per_glyph(BaseData& input, Fnc was_canceled)
{
    // method use square of coord stored into int64_t
    // static_assert(std::is_same<Point::coord_type, int32_t>());
    const Domain::EmbossShape& shape = input.shape_provider->get_shape();
    if (shape.shapes_with_ids.empty())
        return {};

    // Precalculate bounding boxes of glyphs
    // Separate lines of text to vector of Bounds
    assert(get_count_lines(shape.shapes_with_ids) == input.shape_provider->get_text_lines().size());
    size_t count_lines                 = input.shape_provider->get_text_lines().size();
    std::vector<Domain::BoundingBoxes2crd> bbs = create_line_bounds(shape.shapes_with_ids, count_lines);

    double depth  = shape.projection.depth / shape.scale;
    auto scale_tr = Eigen::Scaling(shape.scale);

    size_t s_i_offset = 0; // shape index offset(for next lines)
    indexed_triangle_set result;
    for (size_t text_line_index = 0; text_line_index < input.shape_provider->get_text_lines().size(); ++text_line_index)
    {
        const Domain::BoundingBoxes2crd& line_bbs = bbs[text_line_index];
        const TextLine& line              = input.shape_provider->get_text_lines()[text_line_index];
        Biz::Emboss::PolygonPoints samples = sample_slice(line, line_bbs, shape.scale);
        std::vector<double> angles         = calculate_angles(line_bbs, samples, line.polygon);

        for (size_t i = 0; i < line_bbs.size(); ++i) {
            const Domain::BoundingBox2crd& letter_bb = line_bbs[i];
            if (!letter_bb.defined)
                continue;

            Domain::Vec2d to_zero_vec = Biz::Algorithms::BoundingBox::center(letter_bb).cast<double>()
                * shape.scale; // [in mm]
            float surface_offset = input.is_outside ? -SAFE_SURFACE_OFFSET : (-shape.projection.depth + SAFE_SURFACE_OFFSET);

            if (input.from_surface.has_value())
                surface_offset += *input.from_surface;

            Eigen::Translation<double, 3> to_zero(-to_zero_vec.x(), 0., static_cast<double>(surface_offset));

            const double& angle = angles[i];
            Eigen::AngleAxisd rotate(angle + M_PI_2, Domain::Vec3d::UnitY());

            const Biz::Emboss::PolygonPoint& sample = samples[i];
            Domain::Vec2d offset_vec = Biz::Algorithms::Scaling::unscaled<double>(sample.point
            ); // [in mm]
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

            Domain::BoundingBox2crd bbox = get_extents(line.polygon);
            std::string file_path = "C:/data/temp/line" + std::to_string(text_line_index) + "_letter_position.svg";
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
Domain::TriangleMesh try_create_mesh(BaseData& input, const Fnc& was_canceled)
{
    if (!input.shape_provider->get_text_lines().empty()) {
        Domain::TriangleMesh tm = create_mesh_per_glyph(input, was_canceled);
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
    Domain::EmbossShape& es = input.shape_provider->get_shape();
    double scale    = es.scale;
    double depth    = es.projection.depth / scale;
    auto projectZ   = std::make_unique<ProjectZ>(depth);
    float offset = input.is_outside ? -SAFE_SURFACE_OFFSET : (SAFE_SURFACE_OFFSET - es.projection.depth);
    if (input.from_surface.has_value())
        offset += *input.from_surface;
    Domain::Transform3d tr = Eigen::Translation<double, 3>(0., 0., static_cast<double>(offset)) * Eigen::Scaling(scale);
    ProjectTransform project(std::move(projectZ), tr);
    if (was_canceled())
        return {};
    return Biz::Algorithms::TriangleMesh::construct(polygons2model(shapes, project));
}

template <typename Fnc>
Domain::TriangleMesh create_mesh(BaseData& input, const Fnc& was_canceled)
{
    // It is neccessary to create some shape
    // Emboss text window is opened by creation new emboss text object
    Domain::TriangleMesh result = try_create_mesh(input, was_canceled);
    if (was_canceled())
        return {};

    if (result.its.empty()) {
        result = create_default_mesh();
        if (was_canceled())
            return {};
        // only info
        // ctl.call_on_main_thread([]() {
        create_message("It is used default volume for embossed text, try to change text or font to fix it.");
        //});
    }

    assert(!result.its.empty());
    return result;
}

Domain::TriangleMesh create_default_mesh()
{
    return Biz::Algorithms::TriangleMesh::construct(Biz::Algorithms::TriangleMesh::its_make_cube(36., 4., 2.5));
    //// When cant load any font use default object loaded from file
    // std::string  path = Slic3r::resources_dir() + "/data/embossed_text.obj";
    // Domain::TriangleMesh triangle_mesh;
    // if (!load_obj(path.c_str(), &triangle_mesh)) {
    // // when can't load mesh use cube
    // return Biz::Algorithms::TriangleMesh::construct(
    // Biz::Algorithms::TriangleMesh::its_make_cube(36., 4., 2.5));
    //}
    // return triangle_mesh;
}

Domain::ModelVolume* get_volume(Domain::Project& project, const Domain::ObjectID& volume_id)
{
    const Domain::Model& model = project.model();
    for (const Domain::ModelObject* object_ptr : model.objects) {
        for (Domain::ModelVolume* volume_ptr : object_ptr->volumes) {
            if (volume_ptr->id() == volume_id)
                return volume_ptr;
        }
    }
    return nullptr;
}

void final_update_volume(Domain::TriangleMesh&& mesh, UpdateVolumeParams& data)
{
    // all used symbol in text conain only empty glyphs
    if (mesh.its.empty())
        return create_message("Empty mesh can't be created.");

    // select project
    Biz::ProjectInteractor& project_interactor = data.base.project_interactor;
    if (project_interactor.selected_project_id() != data.base.project_id)
        project_interactor.select_project(data.base.project_id);
    Domain::Project& project        = project_interactor.selected_project();
    Domain::ModelVolume* volume_ptr = get_volume(project, data.volume_id);
    // could appear when user delete edited volume
    if (volume_ptr == nullptr)
        return;
    Domain::ModelVolume& volume = *volume_ptr;

    if (data.volume_trmat.has_value()) {
        volume.set_transformation(*data.volume_trmat);
    } else {
        // apply fix matrix made by store to .3mf
        const std::optional<Domain::EmbossShape>& emboss_shape = volume.emboss_shape;
        assert(emboss_shape.has_value());
        if (emboss_shape.has_value() && emboss_shape->fix_3mf_tr.has_value())
            volume.set_transformation(volume.get_matrix() * emboss_shape->fix_3mf_tr->inverse());
    }
    if (data.volume_type.has_value()) {
        volume.set_type(*data.volume_type);
    }

    UpdateJob::update_volume(volume, std::move(mesh), data.base);
}

void
create_volume(Domain::TriangleMesh&& mesh, const Domain::SelectionId& project_id, const Domain::ObjectID& object_id, const Domain::ModelVolumeType type, const std::optional<Domain::Transform3d>& trmat, const BaseData& data, int /*GLGizmosManager::EType*/ gizmo)
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
        size_t instance_index = 0; // must exist
        Domain::BoundingBox3d instance_bb = Biz::Algorithms::ModelObject::instance_bounding_box(*obj, instance_index);
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
        Domain::Transform3d tr = obj->instances.front()->get_transformation().get_matrix_no_offset().inverse(
        );
        Domain::Transform3d volume_trmat = tr * Eigen::Translation3d(offset_tr);
        vol->set_transformation(volume_trmat);
    }
    data.shape_provider->write(*vol);
    data.project_interactor.scene_interactor().add_volume(vol);
}

OrthoProject create_projection_for_cut(Domain::Transform3d tr, double shape_scale, const std::pair<float, float>& z_range)
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

OrthoProject3d create_emboss_projection(bool is_outside, float emboss, Domain::Transform3d tr, Biz::CGAL::Algorithms::SurfaceCut& cut)
{
    float front_move = (is_outside) ? emboss : SAFE_SURFACE_OFFSET, back_move = -((is_outside) ? SAFE_SURFACE_OFFSET : emboss);
    its_transform(cut, tr.pretranslate(Domain::Vec3d(0., 0., front_move)));
    Domain::Vec3d from_front_to_back(0., 0., back_move - front_move);
    return OrthoProject3d(from_front_to_back);
}

/// <summary>
/// Check whether transformation matrix contains odd number of mirroring.
/// NOTE: In code is sometime function named is_left_handed
/// </summary>
/// <param name="transform">Transformation to check</param>
/// <returns>Is positive determinant</returns>
bool has_reflection(const Domain::Transform3d& transform) { return transform.linear().determinant() < 0; }

indexed_triangle_set
cut_surface_to_its(const Domain::ExPolygons& shapes, const Domain::Transform3d& tr, const ModelSources& sources, BaseData& input, std::function<bool()> was_canceled)
{
    assert(!sources.empty());
    BoundingBox bb     = Biz::Algorithms::ExPolygon::get_extents(shapes);
    double shape_scale = input.shape_provider->get_shape().scale;

    const ModelSource* biggest = &sources.front();

    size_t biggest_count = 0;
    // convert index from (s)ources to (i)ndexed (t)riangle (s)ets
    std::vector<size_t> s_to_itss(sources.size(), std::numeric_limits<size_t>::max());
    std::vector<indexed_triangle_set> itss;
    itss.reserve(sources.size());
    for (const ModelSource& s : sources) {
        Domain::Transform3d mesh_tr_inv       = s.tr.inverse();
        Domain::Transform3d cut_projection_tr = mesh_tr_inv * tr;
        std::pair<float, float> z_range{0., 1.};
        OrthoProject cut_projection = create_projection_for_cut(cut_projection_tr, shape_scale, z_range);
        // copy only part of source model
        indexed_triangle_set its = Biz::CGAL::Algorithms::its_cut_AoI(s.mesh->its, bb, cut_projection);
        if (its.indices.empty())
            continue;
        if (biggest_count < its.vertices.size()) {
            biggest_count = its.vertices.size();
            biggest       = &s;
        }
        size_t source_index     = &s - &sources.front();
        size_t its_index        = itss.size();
        s_to_itss[source_index] = its_index;
        itss.emplace_back(std::move(its));
    }
    if (itss.empty())
        return {};

    Domain::Transform3d tr_inv            = biggest->tr.inverse();
    Domain::Transform3d cut_projection_tr = tr_inv * tr;

    size_t itss_index     = s_to_itss[biggest - &sources.front()];
    BoundingBoxf3 mesh_bb = Domain::bounding_box(itss[itss_index]);
    for (const ModelSource& s : sources) {
        itss_index = s_to_itss[&s - &sources.front()];
        if (itss_index == std::numeric_limits<size_t>::max())
            continue;
        if (&s == biggest)
            continue;

        Domain::Transform3d tr    = s.tr * tr_inv;
        bool fix_reflected        = true;
        indexed_triangle_set& its = itss[itss_index];
        its_transform(its, tr, fix_reflected);
        BoundingBoxf3 its_bb = Domain::bounding_box(its);
        mesh_bb = Biz::Algorithms::BoundingBox::merge(mesh_bb, its_bb);
    }

    // tr_inv = transformation of mesh inverted
    Domain::Transform3d emboss_tr    = cut_projection_tr.inverse();
    BoundingBoxf3 mesh_bb_tr = Biz::Algorithms::BoundingBox::transformed(mesh_bb, emboss_tr);
    std::pair<float, float> z_range{mesh_bb_tr.min.z(), mesh_bb_tr.max.z()};
    OrthoProject cut_projection = create_projection_for_cut(cut_projection_tr, shape_scale, z_range);
    float projection_ratio = (-z_range.first + safe_extension) / (z_range.second - z_range.first + 2 * safe_extension);

    Domain::ExPolygons shapes_data; // is used only when text is reflected to reverse polygon points order
    const Domain::ExPolygons* shapes_ptr = &shapes;
    bool is_text_reflected       = has_reflection(tr);
    if (is_text_reflected) {
        // revert order of points in expolygons
        // CW --> CCW
        shapes_data = shapes; // copy
        for (Domain::ExPolygon& shape : shapes_data) {
            shape.contour.reverse();
            for (Domain::Polygon& hole : shape.holes)
                hole.reverse();
        }
        shapes_ptr = &shapes_data;
    }

    // Use CGAL to cut surface from triangle mesh
    Biz::CGAL::Algorithms::SurfaceCut cut = Biz::CGAL::Algorithms::cut_surface(*shapes_ptr, itss, cut_projection, projection_ratio);

    if (is_text_reflected) {
        for (Biz::CGAL::Algorithms::SurfaceCut::Contour& c : cut.contours)
            std::reverse(c.begin(), c.end());
        for (Domain::Index3& t : cut.indices)
            std::swap(t[0], t[1]);
    }

    if (cut.empty())
        return {}; // There is no valid surface for text projection.
    if (was_canceled())
        return {};

    // !! Projection needs to transform cut
    double depth = input.shape_provider->get_projection().depth;
    OrthoProject3d projection = create_emboss_projection(input.is_outside, depth, emboss_tr, cut);
    return cut2model(cut, projection);
}

Domain::TriangleMesh cut_per_glyph_surface(BaseData& input1, const SurfaceVolumeData& input2, std::function<bool()> was_canceled)
{
    // Precalculate bounding boxes of glyphs
    // Separate lines of text to vector of Bounds
    const Domain::EmbossShape& es = input1.shape_provider->get_shape();
    if (was_canceled())
        return {};
    if (es.shapes_with_ids.empty())
        throw JobException(_u8L("Font doesn't have any shape for given text.").c_str());

    const Biz::Emboss::TextLines& text_lines = input1.shape_provider->get_text_lines();
    assert(get_count_lines(es.shapes_with_ids) == text_lines.size());
    size_t count_lines             = text_lines.size();
    std::vector<BoundingBoxes> bbs = create_line_bounds(es.shapes_with_ids, count_lines);

    size_t s_i_offset = 0; // shape index offset(for next lines)
    indexed_triangle_set result;
    for (size_t text_line_index = 0; text_line_index < text_lines.size(); ++text_line_index) {
        const BoundingBoxes& line_bbs = bbs[text_line_index];
        const TextLine& line          = text_lines[text_line_index];
        PolygonPoints samples         = sample_slice(line, line_bbs, es.scale);
        std::vector<double> angles    = calculate_angles(line_bbs, samples, line.polygon);

        for (size_t i = 0; i < line_bbs.size(); ++i) {
            const BoundingBox& glyph_bb = line_bbs[i];
            if (!glyph_bb.defined)
                continue;

            const double& angle = angles[i];
            auto rotate         = Eigen::AngleAxisd(angle + M_PI_2, Domain::Vec3d::UnitY());

            const PolygonPoint& sample = samples[i];
            Domain::Vec2d offset_vec = Biz::Algorithms::Scaling::unscaled<double>(sample.point); // [in mm]
            auto offset_tr = Eigen::Translation<double, 3>(offset_vec.x(), 0., -offset_vec.y());

            Domain::ExPolygons glyph_shape = es.shapes_with_ids[s_i_offset + i].expoly;
            assert(Biz::Algorithms::ExPolygon::get_extents(glyph_shape) == glyph_bb);

            Domain::Point offset(-Biz::Algorithms::BoundingBox::center(glyph_bb).x(), 0);
            for (Domain::ExPolygon& s : glyph_shape)
                s.translate(offset);

            Domain::Transform3d modify = offset_tr * rotate;
            Domain::Transform3d tr     = input2.transform * modify;
            indexed_triangle_set glyph_its = cut_surface_to_its(glyph_shape, tr, input2.sources, input1, was_canceled);
            // move letter in volume on the right position
            its_transform(glyph_its, modify);

            // Improve: union instead of merge
            Domain::its_merge(result, std::move(glyph_its));

            if (((s_i_offset + i) % 15) && was_canceled())
                return {};
        }
        s_i_offset += line_bbs.size();
    }

    if (was_canceled())
        return {};
    if (result.empty())
        throw JobException(_u8L("There is no valid surface for text projection.").c_str());
    return Domain::TriangleMesh(std::move(result));
}

// input can't be const - cache of font
template <typename Fnc>
Domain::TriangleMesh cut_surface(BaseData& input1, const SurfaceVolumeData& input2, const Fnc& was_canceled)
{
    if (!input1.shape_provider->get_text_lines().empty())
        return cut_per_glyph_surface(input1, input2, was_canceled);

    Domain::ExPolygons shapes = create_shape(*input1.shape_provider, was_canceled);
    if (was_canceled())
        return {};
    if (shapes.empty())
        throw JobException(_u8L("Font doesn't have any shape for given text.").c_str());

    indexed_triangle_set its = cut_surface_to_its(shapes, input2.transform, input2.sources, input1, was_canceled);
    if (was_canceled())
        return {};
    if (its.empty())
        throw JobException(_u8L("There is no valid surface for text projection.").c_str());

    return Biz::Algorithms::TriangleMesh::construct(std::move(its));
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
        const Domain::TriangleMesh& tm = v->mesh();
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

bool queue_job(std::unique_ptr<Job> job)
{
    std::function<std::unique_ptr<Job>(StopToken, std::unique_ptr<Job>&&)> process =
        [](StopToken stop_token, std::unique_ptr<Job>&& job) -> std::unique_ptr<Job>
    {
        job->process(stop_token);
        return job;
    };
    std::function<void(std::unique_ptr<Job>&&)> finalize = [](std::unique_ptr<Job>&& job)
    { job->finalize(); };

    Biz::Platform::PlatformServices::instance()
        .job_manager()
        .create_job("EmbossJob", process, std::move(job))
        .on_result(finalize)
        .start();
    return true;
}

bool start_create_volume_job(const Domain::ModelObject& object, const std::optional<Domain::Transform3d>& volume_tr, BaseData& data, Domain::ModelVolumeType volume_type)
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
            CreateSurfaceVolumeData
                surface_data{std::move(sfvd), std::move(data), volume_type, object.id()};
            job = std::make_unique<CreateSurfaceVolumeJob>(std::move(surface_data));
        }
    }
    if (!use_surface) {
        // create volume
        DataCreateVolume create_volume_data{std::move(data), volume_type, object.id(), volume_tr};
        job = std::make_unique<CreateVolumeJob>(std::move(create_volume_data));
    }
    return queue_job(std::move(job));
}

bool start_create_object_job(CreateVolumeParams& input, const Domain::Vec2d& coor)
{
    // create transformation on the coordinate
    DataCreateObject data{.base = std::move(input.base), .bed_coor = coor, .angle = input.angle};
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

void create_message(const std::string& message)
{
    // TODO: show the message

    // show_error(nullptr, message.c_str());
}

} // namespace
