#include "Slic3r/App/Plater/MMPaintedVolumeRendering.hpp"

#include <algorithm>
#include <ranges>

#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Scene/TextureUnits.hpp"
#include "Slic3r/Biz/Algorithms/FacetsAnnotation.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/PixelFormat.hpp"
#include "Slic3r/Domain/TriangleMesh.hpp"
#include "Slic3r/Domain/TriangleSelector.hpp"

using Slic3r::App::Render::Device;
using Slic3r::App::Render::Geometry;
using Slic3r::App::Render::GeometryBuilder;
using Slic3r::App::Render::Material;
using Slic3r::App::Render::MaterialTextures;
using Slic3r::App::Render::PrimitiveType;
using Slic3r::App::Render::TextureMagFilter;
using Slic3r::App::Render::TextureMinFilter;
using Slic3r::App::Render::TexturePtr;
using Slic3r::App::Render::TextureWrap;
using Slic3r::App::Render::VertexP3N3I1;
using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::indexed_triangle_set_with_color;
using Slic3r::Domain::ModelVolume;
using Slic3r::Domain::PixelFormat;
using Slic3r::Domain::Vec3f;
using Slic3r::Domain::TriangleSelector::TriangleStateType;

using namespace Slic3r::Biz;

namespace Slic3r::App::Plater::MMPainting {

MMPaintedVolumeGeometryId mm_painted_volume_geometry_id(const ModelVolume& model_volume)
{
    return {model_volume.id().id, model_volume.mm_segmentation_facets.timestamp()};
}

std::unique_ptr<Geometry>
create_mm_painted_volume_geometry(Device& device, const ModelVolume& model_volume)
{
    const constexpr int paint_state_count = static_cast<int>(TriangleStateType::Count);

    const indexed_triangle_set_with_color painted_mesh =
        Algorithms::FacetsAnnotation::get_all_facets_strict_with_colors(
            model_volume.mm_segmentation_facets,
            model_volume
        );

    const size_t vertex_count = painted_mesh.indices.size() * 3;

    GeometryBuilder<VertexP3N3I1> geometry_builder;
    geometry_builder.reserve(vertex_count, 0);
    for (const stl_triangle_vertex_indices& triangle : painted_mesh.indices) {
        const size_t triangle_idx = &triangle - painted_mesh.indices.data();
        const int paint_state     = triangle_idx < painted_mesh.colors.size() ?
                static_cast<int>(painted_mesh.colors[triangle_idx]) :
                0;

        ASSERT(paint_state >= 0 && paint_state < paint_state_count);

        const float palette_index = static_cast<float>(paint_state);
        const stl_vertex& v0      = painted_mesh.vertices[triangle[0]];
        const stl_vertex& v1      = painted_mesh.vertices[triangle[1]];
        const stl_vertex& v2      = painted_mesh.vertices[triangle[2]];
        const Vec3f normal        = (v1 - v0).cross(v2 - v0).normalized();
        geometry_builder.add_vertex({v0, normal, palette_index})
            .add_vertex({v1, normal, palette_index})
            .add_vertex({v2, normal, palette_index});
    }

    geometry_builder.add_draw_command({PrimitiveType::Triangles, 0, vertex_count, Material{}});

    return geometry_builder.build(device);
}

std::vector<ColorRGBA>
create_palette_colors(const ColorRGBA& default_color, const std::vector<ColorRGBA>& slot_colors)
{
    constexpr size_t palette_size = static_cast<size_t>(TriangleStateType::Count);

    std::vector<ColorRGBA> palette_colors(palette_size, default_color);
    std::ranges::copy(slot_colors | std::views::take(palette_size - 1), palette_colors.begin() + 1);

    return palette_colors;
}

void apply_mm_palette_to_material(
    Device& device,
    const std::vector<ColorRGBA>& palette_colors,
    Material& material
)
{
    const constexpr size_t palette_unit = Scene::TextureUnits::MULTI_MATERIAL_PALETTE;
    const int palette_width             = static_cast<int>(palette_colors.size());

    std::vector<unsigned char> texel_data;
    texel_data.reserve(palette_colors.size() * 4);
    for (const ColorRGBA& palette_color : palette_colors) {
        texel_data.push_back(palette_color.r_uchar());
        texel_data.push_back(palette_color.g_uchar());
        texel_data.push_back(palette_color.b_uchar());
        texel_data.push_back(palette_color.a_uchar());
    }

    const auto palette_texture_it = material.textures().find(palette_unit);
    if (palette_texture_it != material.textures().end()
        && palette_texture_it->second->width() == palette_width)
    {
        palette_texture_it->second
            ->set_sub_data(PixelFormat::RGBA8, 0, 0, 0, palette_width, 1, texel_data.data());
    } else {
        TexturePtr palette_texture{device.create_texture()};
        palette_texture->set_object_name("mm_palette");
        palette_texture->set_data(
            PixelFormat::RGBA8,
            0,
            palette_width,
            1,
            texel_data.data(),
            texel_data.size()
        );
        palette_texture->set_filtering(TextureMinFilter::Nearest, TextureMagFilter::Nearest);
        palette_texture->set_wrap_s(TextureWrap::ClampToEdge);
        palette_texture->set_wrap_t(TextureWrap::ClampToEdge);

        material.set_texture(palette_unit, palette_texture);
    }

    material.set_uniform("mm_palette_tex", Scene::TextureUnits::MULTI_MATERIAL_PALETTE);
}

Material create_mm_painted_volume_material(Device& device)
{
    return Material{}
        .set_shader(device.context().shader_manager().shader("mm_gouraud_light"))
        .set_uniform("use_uniform_color", false)
        .set_transparent(false);
}

} // namespace Slic3r::App::Plater::MMPainting
