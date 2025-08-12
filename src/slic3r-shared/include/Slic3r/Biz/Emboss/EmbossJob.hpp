///|/ Copyright (c) Prusa Research 2021 - 2022 Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_EmbossJob_hpp_
#define slic3r_EmbossJob_hpp_

#include <atomic>
#include <memory>
#include <string>
#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Scene/Scene.hpp" // NodePickResults
#include "Slic3r/Biz/Emboss/Emboss.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/TriangleMesh.hpp"
#include "Slic3r/Domain/EmbossShape.hpp" // ExPolygonsWithIds

namespace Slic3r::Biz::Emboss {

/// <summary>
/// Provide ability to lazy create shape for Embossing
/// Text have to load font file and create shapes for glyphs
/// SVG have to load SVG file and create shapes for paths
/// Different store into volume
/// </summary>
class ShapeProvider
{
public:
    virtual ~ShapeProvider() = default;

    /// <summary>
    /// Create shape
    /// e.g. Text extract glyphs from font file
    /// Not 'const' function because it could modify shape
    /// </summary>
    virtual Domain::EmbossShape& get_shape()
    {
        return shape;
    }

    /// <summary>
    /// Write data how to reconstruct shape to volume
    /// </summary>
    /// <param name="volume">Data object for store emboss params</param>
    virtual void write(Domain::ModelVolume& volume) const
    {
        volume.emboss_shape = shape;
    }

    /// <summary>
    /// Used only with text for embossing per glyph
    /// </summary>
    /// <param name="tr">Embossed volume final transformation in world</param>
    /// <param name="vols">Volumes to be sliced to text lines</param>
    /// <returns>True on succes otherwise False(Per glyph shoud be disabled)</returns>
    virtual bool create_text_lines(const Domain::Transform3d& tr, const Domain::ModelVolumePtrs& vols)
    {
        return false;
    }

    /// <summary>
    /// Create text lines when empty
    /// </summary>
    /// <returns></returns>
    virtual const Biz::Emboss::TextLines& get_text_lines()
    {
        return text_lines;
    }

protected:
    Domain::EmbossShape shape;

    // Define per letter projection on one text line
    // [optional] It is not used when empty
    Biz::Emboss::TextLines text_lines = {};
};

using ShapeProviderPtr = std::unique_ptr<ShapeProvider>;

struct BaseData
{
    // Create shape
    ShapeProviderPtr shape_provider;

    // Add volume into project
    Biz::ProjectInteractor& project_interactor;

    // project of the object
    Domain::SelectionId project_id;

    // Define which gizmo open on the success(Text VS SVG)
    uint8_t /* App::Scene::ToolType GLGizmosManager::EType*/ gizmo;

    // Define projection move
    // True (raised) .. move outside from surface (MODEL_PART)
    // False (engraved).. move into object (NEGATIVE_VOLUME & MODIFIER)
    bool is_outside = true;

    // [optional] Define distance for surface
    // It is used only for flat surface (not cutted)
    // Position of Zero(not set value) differ for MODEL_PART and NEGATIVE_VOLUME
    std::optional<float> from_surface;

    // new volume name
    std::string volume_name;
};

/// <summary>
/// shorten params for start_crate_volume functions
/// </summary>
struct CreateVolumeParams
{
    // base input data for job
    BaseData base;

    // New created volume type
    Domain::ModelVolumeType volume_type;

    // Wanted additionl move in Z(emboss) direction of new created volume
    std::optional<float> distance = {};

    // Wanted additionl rotation around Z of new created volume
    std::optional<float> angle = {};
};

/// <summary>
/// Create new volume on position of mouse cursor
/// </summary>
/// <param name="input">Cantain all needed data for start creation job</param>
/// <param name="pick_ray">Ray into scene given by coordinate on screen</param>
/// <param name="picks">Scene Node with intersection of picked ray</param>
/// <returns>True on success otherwise False</returns>
bool start_create_volume(
    CreateVolumeParams& input,
    const App::Scene::Ray& pick_ray,
    const App::Scene::NodePickResults& picks
);

/// <summary>
/// Same as previous function but without mouse position
/// Need to suggest position or put near the selection
/// </summary>
bool start_create_volume_without_position(CreateVolumeParams& input);

/// <summary>
/// Parameters for call start_update_volume function
/// </summary>
struct UpdateVolumeParams
{
    // base input data for job
    BaseData base;

    // Used for prevent flooding Undo/Redo stack on slider.
    bool make_snapshot;

    // Transformation of volume after update volume shape
    // NOTE: Add for style change, because it change rotation and distance from surface
    std::optional<Domain::Transform3d> trmat;
};

/// <summary>
/// Start job for update embossed volume
/// </summary>
/// <param name="data">define update data</param>
/// <returns>True when start job otherwise false</returns>
bool start_update_volume(UpdateVolumeParams& data);

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

} // namespace Slic3r::Biz::Emboss

#endif // slic3r_EmbossJob_hpp_
