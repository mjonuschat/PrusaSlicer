#include "Slic3r/App/Preview/PreviewSceneRenderCustomizer.hpp"
#include "Slic3r/App/Preview/PreviewSceneLayer.hpp"

namespace Slic3r::App::Preview {

  void PreviewSceneRenderCustomizer::on_opaque_pass_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx)
{
    PreviewSceneLayer id = PreviewSceneLayer(layer_idx);

    cmd_buf.set_blending_enabled(false);
    cmd_buf.set_depth_write_enabled(true);
    cmd_buf.set_depth_test_enabled(true);
    if (id != PreviewSceneLayer::Toolpaths)
        cmd_buf.set_cull_face_enabled(true);
}

void PreviewSceneRenderCustomizer::on_transparent_pass_begin(
    Render::CommandBuffer& cmd_buf, size_t layer_index
)
{
    cmd_buf.set_depth_test_enabled(true);
    cmd_buf.set_blending_enabled(true);
    Render::Blending blending { {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha}};
    cmd_buf.set_blending(blending);
    cmd_buf.set_depth_write_enabled(false);
    cmd_buf.set_cull_face_enabled(false);
}

void PreviewSceneRenderCustomizer::on_transparent_pass_end(
    Render::CommandBuffer& cmd_buf, size_t layer_index
)
{
    cmd_buf.set_blending_enabled(false);
    cmd_buf.set_depth_write_enabled(true);
}

void PreviewSceneRenderCustomizer::on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx)
{
    PreviewSceneLayer id = PreviewSceneLayer(layer_idx);
    if (id == PreviewSceneLayer::CogMarker)
        cmd_buf.set_depth_test_enabled(false);
    else if (id == PreviewSceneLayer::Toolpaths)
        cmd_buf.set_cull_face_enabled(false);
}

void PreviewSceneRenderCustomizer::on_layer_end(Render::CommandBuffer& cmd_buf, size_t layer_idx)
{
    PreviewSceneLayer id = PreviewSceneLayer(layer_idx);
    if (id == PreviewSceneLayer::CogMarker)
        cmd_buf.set_depth_test_enabled(true);
    else if (id == PreviewSceneLayer::Toolpaths)
        cmd_buf.set_cull_face_enabled(true);
}

} // namespace Slic3r::App::Preview
