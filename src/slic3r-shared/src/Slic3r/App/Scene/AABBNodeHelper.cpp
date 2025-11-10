#include "Slic3r/App/Scene/AABBNodeHelper.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"

#include <magic_enum/magic_enum.hpp>
#include <fmt/format.h>

namespace Slic3r::App::Scene {

// X axis -> Left / Right
// Y axis -> Bottom / Top
// Z axis -> Floor / Ceil
enum class CornerTag : uint8_t
{
    LBF = 1, // Left Bottom Floor
    RBF, // Right Bottom Floor
    LTF, // Left Top Floor
    RTF, // Right Top Floor
    LBC, // Left Bottom Ceil
    RBC, // Right Bottom Ceil
    LTC, // Left Top Ceil
    RTC  // Right Top Ceil
};

static std::vector<Domain::Vec3f> corner_lines(CornerTag tag)
{
    std::vector<Domain::Vec3f> ret;
    Domain::Vec3f origin = Domain::Vec3f(0.0f, 0.0f, 0.0f);
    switch (tag)
    {
    case CornerTag::LBF:
    {
        ret = {
            origin, origin + 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin + 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin + 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::RBF:
    {
        ret = {
            origin, origin - 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin + 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin + 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::LTF:
    {
        ret = {
            origin, origin + 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin - 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin + 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::RTF:
    {
        ret = {
            origin, origin - 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin - 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin + 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::LBC:
    {
        ret = {
            origin, origin + 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin + 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin - 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::RBC:
    {
        ret = {
            origin, origin - 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin + 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin - 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::LTC:
    {
        ret = {
            origin, origin + 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin - 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin - 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    case CornerTag::RTC:
    {
        ret = {
            origin, origin - 0.5f * Domain::Vec3f::UnitX(), // X axis
            origin, origin - 0.5f * Domain::Vec3f::UnitY(), // Y axis
            origin, origin - 0.5f * Domain::Vec3f::UnitZ(), // Z axis
        };
        break;
    }
    }
    return ret;
}

static Domain::Vec3d corner_offset(CornerTag tag)
{
    switch (tag)
    {
    case CornerTag::LBF: { return Domain::Vec3d::Zero(); }
    case CornerTag::RBF: { return Domain::Vec3d::UnitX(); }
    case CornerTag::LTF: { return Domain::Vec3d::UnitY(); }
    case CornerTag::RTF: { return Domain::Vec3d(1.0, 1.0, 0.0); }
    case CornerTag::LBC: { return Domain::Vec3d::UnitZ(); }
    case CornerTag::RBC: { return Domain::Vec3d(1.0, 0.0, 1.0); }
    case CornerTag::LTC: { return Domain::Vec3d(0.0, 1.0, 1.0); }
    case CornerTag::RTC: { return Domain::Vec3d::Ones(); }
    default: {
        PANIC("Invalid corner tag");
        return Domain::Vec3d::Zero();
    }
    }
}

void build_aabb_node(NodeBuilder& builder, ScenePresenterProjectContext& ctx, Render::Device& device, const std::string& debug_name,
    RenderLayerId layer_id, const Domain::ColorRGB& color)
{
    builder.set_debug_name(fmt::format("{} main", debug_name))
           .set_tag(AABBNodeTag{ 0 });

    auto& geom_mgr = ctx.model_geometry_manager();

    Render::Material material;
    material.set_shader(device.context().shader_manager().shader("flat"))
            .set_uniform("uniform_color", Biz::Algorithms::Color::to_rgba(color));

    constexpr auto cornerTags = magic_enum::enum_values<CornerTag>();
    for (const auto& tag : cornerTags) {
        builder.child(
            [&](NodeBuilder& bldr) {
                AuxiliaryElementId id{ AuxiliaryElementId::Type::AABB, size_t(tag)};
                std::vector<Domain::Vec3f> lines = corner_lines(tag);
                const auto* geom = geom_mgr.get_or_create(id,
                    [&]() { return Render::geometry_from_lines(device, lines); }
                );

                bldr.set_debug_name(fmt::format("{} corner {}", debug_name, magic_enum::enum_name(tag)))
                    .set_tag(AABBNodeTag{ uint8_t(tag) })
                    .transform([&](Domain::Transform3d& xform) { xform.translate(corner_offset(tag)); })
                    .set_mesh(geom, material, layer_id);
            }
        );
    }
}

void update_aabb_node(Node& node, const Eigen::AlignedBox3d& aabb, double edge_coverage_percent,
    std::optional<Domain::ColorRGB> color)
{
    AABBNodeTag* tag = node.tag_of_type<AABBNodeTag>();
    DEBUG_ASSERT(tag != nullptr);
    DEBUG_ASSERT(node.children().size() == magic_enum::enum_count<CornerTag>());
    DEBUG_ASSERT(node.children().front()->has_render_component());
    DEBUG_ASSERT(0.0 < edge_coverage_percent && edge_coverage_percent <= 1.0);

    edge_coverage_percent = std::clamp(edge_coverage_percent, 0.0, 1.0);

    Domain::Vec3d size = aabb.sizes();
    Domain::Vec3d origin = aabb.min();

    auto aaa = size.array();
    bool enabled = std::all_of(size.array().begin(), size.array().end(), [](double comp) { return comp > 0.0; });
    node.set_enabled(enabled);

    Transform main_trafo = Transform::Identity();
    main_trafo.translate(origin);
    node.set_local_transform(main_trafo);

    if (color.has_value()) {
        Domain::ColorRGBA rgba = Biz::Algorithms::Color::to_rgba(*color);
        if (!node.has_material_override()) {
            Render::Material mat = node.children().front()->render_component()->material();
            mat.set_uniform("uniform_color", rgba);
            node.set_material_override(mat);
        }
        else {
            // do not replace material override if color is the same
            // to avoid unneded memory allocations/deallocations
            // deriving from the call to node.set_material_override()
            Render::Material mat = *node.material_override();
            const Render::MaterialUniforms& uniforms = mat.uniforms();
            auto it = uniforms.find("uniform_color");
            DEBUG_ASSERT(it != uniforms.end());
            const Domain::ColorRGBA* stored_color = std::get_if<Domain::ColorRGBA>(&it->second);
            DEBUG_ASSERT(stored_color != nullptr);
            if (rgba != *stored_color) {
                mat.set_uniform("uniform_color", rgba);
                node.set_material_override(mat);
            }
        }
    }
    else
        node.remove_material_override();

    for (auto& child : node.children()) {
        AABBNodeTag* child_node_tag = child->tag_of_type<AABBNodeTag>();
        DEBUG_ASSERT(child_node_tag != nullptr);
        auto child_corner_tag = magic_enum::enum_cast<CornerTag>(child_node_tag->corner_id);
        DEBUG_ASSERT(child_corner_tag.has_value());
        Transform child_trafo = Transform::Identity();
        child_trafo.translate(corner_offset(child_corner_tag.value()).cwiseProduct(size));
        child_trafo.scale(edge_coverage_percent * size);
        child->set_local_transform(child_trafo);
    }
}

void update_aabb_node(Node& node, const Domain::BoundingBox3d& aabb, double edge_coverage_percent,
    std::optional<Domain::ColorRGB> color)
{
    update_aabb_node(node, Eigen::AlignedBox3d(aabb.min, aabb.max), edge_coverage_percent, color);
}

} // namespace Slic3r::App::Scene
