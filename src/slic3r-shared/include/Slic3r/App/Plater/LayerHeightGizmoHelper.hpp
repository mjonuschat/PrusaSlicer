#pragma once

#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/Domain/LayerHeightProfile.hpp"

#include <vector>

namespace Slic3r::Biz::Scene {
struct ObjectSelection;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::Domain {
class ConfigContainer;
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

struct LayerHeightParams
{
    /**
     * The regular layer height, applied for all but the first layer, if not overridden by layer ranges
     * or by the variable layer thickness table.
     */
    double layer_height{0.};
    /**
     * Minimum layer height, to be used for the automatic adaptive layer height algorithm,
     * or by an interactive layer height editor.
     */
    double min_layer_height{0.};
    /**
     * Maximum layer height, to be used for the automatic adaptive layer height algorithm,
     * or by an interactive layer height editor.
     */
    double max_layer_height{0.};
    /**
     * Thickness of the first layer. This is either the first print layer thickness if printed without a raft,
     * or a bridging flow thickness if printed over a non-soluble raft,
     * or a normal layer height if printed over a soluble raft.
     */
    double first_object_layer_height{0.};
    /**
     * Is the 1st object layer height fixed, or could it be varied?
     */
    bool first_object_layer_height_fixed{false};
    /**
     * Height of the object to be printed. This value does not contain the raft height.
     * This value is scaled by shrinkage compensation in the Z-axis.
     */
    double object_print_z_height{0.};
    /**
     * Height of the object to be printed. This value does not contain the raft height.
     * This value isn't scaled by shrinkage compensation in the Z-axis.
     */
    double object_print_z_uncompensated_height{0.};
    /**
     * Scaling factor for compensating shrinkage in Z-axis.
     */
    double object_shrinkage_compensation_z{1.0};
    /**
     * Current layer height profile.
     */
    Domain::ZHeightPairs layer_height_profile;
};

LayerHeightParams compute_layer_height_params(
    const Biz::Scene::ObjectSelection& object_selection,
    const Domain::Project& project,
    const Domain::ConfigContainer& config_container
);

/**
 * CPU-side texture data for layer height visualization.
 */
struct LayerHeightTexture
{
    /**
     * Texture data buffer (RGBA).
     */
    std::vector<char> data;
    /**
     * Width of the texture, top level.
     */
    size_t width{0};
    /**
     * Height of the texture, top level.
     */
    size_t height{0};
    /**
     * For how many levels of detail is the data allocated?
     */
    size_t levels{0};
    /**
     * Number of texture cells allocated for the height texture.
     */
    size_t cells{0};

    /**
     * Allocate the texture data buffer based on width, height, and levels.
     */
    void allocate()
    {
        size_t size = width * height * 4; // LOD 0: RGBA
        if (levels >= 2) {
            size += (width / 2) * (height / 2) * 4; // LOD 1: half resolution
        }

        ASSERT(levels <= 2);
        data.assign(size, 0);
    }
};

/**
 * Generate layer height texture from layer height profile.
 *
 * Produces a 2D RGBA texture visualizing layer heights using a color gradient.
 * Supports up to 2 LOD levels for mipmap-based rendering.
 *
 * @param layers Layer height profile as pairs (z_bottom, z_top).
 * @param min_layer_height Minimum allowed layer height.
 * @param max_layer_height Maximum allowed layer height.
 * @param layer_height Default layer height.
 * @param object_height Total Z height of the object.
 * @return LayerHeightTexture with allocated and filled texture data.
 */
LayerHeightTexture generate_layer_height_texture(
    const Domain::LayerZRanges& layers,
    double min_layer_height,
    double max_layer_height,
    double layer_height,
    double object_height
);

class LayerHeightMaterialWrapper
{
public:
    LayerHeightMaterialWrapper()  = default;
    ~LayerHeightMaterialWrapper() = default;

    void init(Render::Device& device);

    void reset();

    /**
     * Get the material for applying to scene nodes.
     */
    const Render::Material& material() const;

    /**
     * Set layer data and regenerate texture.
     *
     * @param layers Layer height profile as pairs (z_bottom, z_top).
     * @param min_layer_height Minimum allowed layer height.
     * @param max_layer_height Maximum allowed layer height.
     * @param layer_height Default layer height.
     * @param object_height Total Z height of the object.
     * @param object_max_z Maximum Z of the object.
     */
    void set_layers(
        const Domain::LayerZRanges& layers,
        double min_layer_height,
        double max_layer_height,
        double layer_height,
        double object_height,
        float object_max_z
    );

    /**
     * Set the cursor Z position.
     */
    void set_cursor_z(float cursor_z);

    /**
     * Set the cursor band width.
     */
    void set_cursor_band_width(float band_width);

private:
    Render::Material m_material;
    Render::TexturePtr m_texture;
};

} // namespace Slic3r::App::Plater
