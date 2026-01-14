///|/ Copyright (c) Prusa Research 2021 - 2026 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_EmbossJob_hpp_
#define slic3r_EmbossJob_hpp_

#include <memory>
#include <string>
#include "Slic3r/Biz/Emboss/Emboss.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
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
    explicit ShapeProvider(const Domain::EmbossShape& shape, TextLines text_lines = {}):
        m_shape(shape), // copy
        m_text_lines(text_lines)
    {}

    virtual ~ShapeProvider() = default;

    /**
    @brief Write data how to reconstruct shape to volume
    @param volume Data object for store emboss params
    */
    virtual void write(Domain::ModelVolume& volume) const
    {
        volume.emboss_shape = m_shape;

        // Fix for object: stored attribute that volume use surface when it is object
        // Can appear when remove source volume in the object
        if (volume.is_the_only_one_part() &&
            m_shape.projection.use_surface) {
            volume.emboss_shape->projection.use_surface = false;
        }
    }

    /**
    @brief Used only with text for embossing per glyph
           \note Only for new volume creation(without ui)
    @param tr Embossed volume final transformation in world
    @param object Contain volumes to be sliced to text lines
    @return True on succes otherwise False(Per glyph shoud be disabled)
    */
    virtual void create_text_lines(
        const Domain::Transform3d& tr, 
        const Domain::ModelObject& object) {}

    /**
    @brief Text extract glyphs from font file
    @param make_union Flag, when true union of expolygon is forced to calculate
    @return True on succes otherwise False
    */
    virtual bool create_shape(){ return !m_shape.final_shape.expolygons.empty(); }
    bool create_shape_with_union() {
        if (!create_shape())
            return false;

        // IMPROVE: use real size of volume for union delta value
        // ... need world matrix for volume
        // ... printer resolution will be fine too
        union_with_delta(m_shape, UNION_DELTA, UNION_MAX_ITERATIN);
        return !m_shape.final_shape.expolygons.empty();
    }

    const Domain::EmbossShape& get_shape() const { return m_shape; }
    const TextLines& get_text_lines() const { return m_text_lines; }
    const Domain::EmbossProjection& get_projection() const { return m_shape.projection; }

protected:
    Domain::EmbossShape m_shape;

    // Define per letter projection on one text line
    // [optional] It is not used when empty
    TextLines m_text_lines = {};
};

using ShapeProviderPtr = std::unique_ptr<ShapeProvider>;

enum class JobIssue {
    canceled, // canceled thread
    no_shape, // Font doesn't have any shape for given text
    no_surface, // There is no valid surface for text projection
    default_volume // Create function created default shape (shape was not used)
};

struct BaseData
{
    // Create shape
    ShapeProviderPtr shape_provider;

    // Add volume into project
    // @janBartipan garanted it will be alive in the finalize part of the job.
    // Job manager will not call finalize when Project interactor is not alive.
    Biz::ProjectInteractor& project_interactor;

    // Project of the object
    Domain::SelectionId project_id;

    // Define projection move
    // True (raised) .. move outside from surface (MODEL_PART)
    // False (engraved).. move into object (NEGATIVE_VOLUME & MODIFIER)
    bool is_outside = true;

    // [optional] Define distance for surface
    // It is used only for flat surface (not cutted)
    // Position of Zero(not set value) differ for MODEL_PART and NEGATIVE_VOLUME
    std::optional<float> per_glyph_surface_distance;

    // new volume name
    std::string volume_name;

    // function called on main thread when issue during proccess job appear
    using IssueFn = std::function<void(JobIssue)>;
    IssueFn issue_fn;
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

bool start_create_object_job(CreateVolumeParams& input, const Domain::Vec2d& coor);

/**
@brief Start job for add new volume to object with given transformation
@param instance Define where to add
@param volume_tr Wanted volume transformation
@param data Define what to emboss - shape
@param volume_type Type of volume: Part, negative, modifier
@return Nullptr when job is sucessfully add to worker otherwise return data to be processed different way
*/
bool start_create_volume_job(const Domain::ModelInstance& instance, const Domain::Transform3d& volume_tr, BaseData& data, Domain::ModelVolumeType volume_type);

/**
@brief Parameters for call start_update_volume function
*/
struct UpdateVolumeParams
{
    // base input data for job
    BaseData base;

    // unique identifier of volume to change
    Domain::ObjectID volume_id;

    Domain::ObjectID instance_id;

    std::optional<Domain::ModelVolumeType> volume_type = std::nullopt;
};

/**
@brief Start job for update embossed volume
@param data define update data
@param volume volume to update
@return True when start job otherwise false
*/
bool start_update_volume(UpdateVolumeParams&& data, const Domain::ModelVolume& volume);

ProjectTransform create_projection(const Domain::EmbossShape& es, bool is_outside);

const Domain::ModelInstance* get_selected_instance(const Domain::ElementRefs& elms, const Domain::Project& project);
const Domain::ModelInstance* get_selected_instance(const Biz::ProjectInteractor& project_interactor);

} // namespace Slic3r::Biz::Emboss

#endif // slic3r_EmbossJob_hpp_
