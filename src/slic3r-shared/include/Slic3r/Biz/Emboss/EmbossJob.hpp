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

/**
@brief Provide ability to lazy create shape for Embossing
Text have to load font file and create shapes for glyphs
SVG have to load SVG file and create shapes for paths
Different store into volume
*/
class ShapeProvider
{
public:
    virtual ~ShapeProvider() = default;

    Domain::EmbossProjection& get_projection() {
        return m_shape.projection;
    }

    /**
    @brief Create shape
    e.g. Text extract glyphs from font file
    Not 'const' function because it could modify shape
    */
    virtual Domain::EmbossShape& get_shape()
    {
        return m_shape;
    }

    /**
    @brief Write data how to reconstruct shape to volume
    @param volume Data object for store emboss params
    */
    virtual void write(Domain::ModelVolume& volume) const
    {
        volume.emboss_shape = m_shape;
    }

    /**
    @brief Used only with text for embossing per glyph
    @param tr Embossed volume final transformation in world
    @param vols Volumes to be sliced to text lines
    @return True on succes otherwise False(Per glyph shoud be disabled)
    */
    virtual bool create_text_lines(const Domain::Transform3d& tr, const Domain::ModelVolumePtrs& vols)
    {
        return false;
    }

    /**
    @brief Create text lines when empty
    */
    virtual const Biz::Emboss::TextLines& get_text_lines()
    {
        return m_text_lines;
    }

protected:
    Domain::EmbossShape m_shape;

    // Define per letter projection on one text line
    // [optional] It is not used when empty
    Biz::Emboss::TextLines m_text_lines = {};
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

/**
@brief shorten params for start_crate_volume functions
*/
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

/**
@brief Create new volume on position of mouse cursor
@param input Cantain all needed data for start creation job
@param pick_ray Ray into scene given by coordinate on screen
@param picks Scene Node with intersection of picked ray
@return True on success otherwise False
*/
bool start_create_volume(
    CreateVolumeParams& input,
    const App::Scene::Ray& pick_ray,
    const App::Scene::NodePickResults& picks
);

/**
@brief Parameters for call start_update_volume function
*/
struct UpdateVolumeParams
{
    // base input data for job
    BaseData base;

    // unique identifier of volume to change
    Domain::ObjectID volume_id;

    // Transformation of volume after update volume shape
    // NOTE: Add for style change, because it change rotation and distance from surface
    std::optional<Domain::Transform3d> volume_trmat = std::nullopt;
    std::optional<Domain::ModelVolumeType> volume_type = std::nullopt;
};

/**
@brief Start job for update embossed volume
@param data define update data
@param volume volume to update
@return True when start job otherwise false
*/
bool start_update_volume(UpdateVolumeParams&& data, const Domain::ModelVolume& volume);

/**
 *  @brief  Find volume by id inside project without known object_id
 *  @note Move functionality into foundable place not only EmbossJob
 *  @param  project   - Project to search for volume_id
 *  @param  volume_id - Define volume(unique inside project)
 *  @retval           - Volume when found otherwise nullptr
 */
Domain::ModelVolume* get_volume(const Domain::Project& project, const Domain::ObjectID& volume_id);

} // namespace Slic3r::Biz::Emboss

#endif // slic3r_EmbossJob_hpp_
