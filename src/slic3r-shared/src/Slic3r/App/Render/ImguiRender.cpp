#include "ImguiRender.hpp"
#include "CommandBuffer.hpp"
#include "Geometry.hpp"
#include "Device.hpp"
#include "Shader.hpp"
#include "ShaderManager.hpp"
#include "Texture.hpp"
#include "MathUtils.hpp"

#include <Slic3r/Assert.hpp>
#include <imgui/imgui.h>

namespace Slic3r::App::Render {

ImguiRender::ImguiRender(Device& device)
    : m_device(device)
    , m_vertex_format({
          {VertexAttribType::Vertex, DataType::Float, 2, IM_OFFSETOF(ImDrawVert, pos)},
          {VertexAttribType::TexCoord0, DataType::Float, 2, IM_OFFSETOF(ImDrawVert, uv)},
          {VertexAttribType::Color, DataType::UByte, 4, IM_OFFSETOF(ImDrawVert, col), true}
      })
{}

void ImguiRender::new_frame()
{
    if (m_shader == nullptr)
        init();
}

void ImguiRender::init()
{
    m_geom = std::make_unique<Geometry>(m_device, BufferUsage::StreamDraw);
    m_shader = m_device.context().shader_manager().get_shader("imgui");
    ASSERT(m_shader != nullptr, "Cannot load imgui shader");
    create_font_texture();
}

void ImguiRender::create_font_texture()
{
    ImGuiIO& io = ImGui::GetIO();
    uint8_t* pixels;
    int width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    m_font_texture = m_device.create_texture();
    m_font_texture->set_data(PixelFormat::RGBA8, 0, width, height, pixels);
    //m_font_texture->set_filtering(Texture::MinFilter::Linear, Texture::MagFilter::Linear);

    io.Fonts->SetTexID(m_font_texture.get());
}


void ImguiRender::setup_state(CommandBuffer& buffer, const ImDrawData* draw_data)
{
    buffer.set_blending({
        {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha},
        {Render::BlendFactor::One, Render::BlendFactor::OneMinusSrcAlpha}
    });
    buffer.set_blending_enabled(true);
    buffer.set_scissor_enabled(true);
    buffer.set_stencil_test_enabled(false);
    buffer.set_cull_face_enabled(false);

    buffer.bind_shader(*m_shader);
//    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
//    glEnable(GL_BLEND);
//    glBlendEquation(GL_FUNC_ADD);
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//    glDisable(GL_CULL_FACE);
//    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_STENCIL_TEST);
//    glEnable(GL_SCISSOR_TEST);
//#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
//    if (g_GlVersion >= 310)
//        glDisable(GL_PRIMITIVE_RESTART);
//#endif
//#ifdef GL_POLYGON_MODE
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//#endif
    const int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    buffer.set_viewport({0, 0, fb_width, fb_height});
    const float left = draw_data->DisplayPos.x;
    const float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const float top = draw_data->DisplayPos.y;
    const float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

    Matrix4f projection = ortho(left, right, bottom, top, -1, 1);
    m_shader->set_uniform("ProjMtx", projection);
    //buffer.bind_geometry(*m_geom, *m_shader);

//    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
//#if defined(GL_CLIP_ORIGIN)
//    bool clip_origin_lower_left = true;
//    if (g_GlVersion >= 450)
//    {
//        GLenum current_clip_origin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&current_clip_origin);
//        if (current_clip_origin == GL_UPPER_LEFT)
//            clip_origin_lower_left = false;
//    }
//#endif
//
//    // Setup viewport, orthographic projection matrix
//    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
//    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
//    float L = draw_data->DisplayPos.x;
//    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
//    float T = draw_data->DisplayPos.y;
//    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
//#if defined(GL_CLIP_ORIGIN)
//    if (!clip_origin_lower_left) { float tmp = T; T = B; B = tmp; } // Swap top and bottom if origin is upper left
//#endif
//    const float ortho_projection[4][4] =
//        {
//            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
//            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
//            { 0.0f,         0.0f,        -1.0f,   0.0f },
//            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
//        };
//    glUseProgram(g_ShaderHandle);
//    glUniform1i(g_AttribLocationTex, 0);
//    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
//
//#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
//    if (g_GlVersion >= 330)
//        glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
//#endif
//
//    (void)vertex_array_object;
//#ifndef IMGUI_IMPL_OPENGL_ES2
//    glBindVertexArray(vertex_array_object);
//#endif
//
//    // Bind vertex/index buffers and setup attributes for ImDrawVert
//    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
//    glEnableVertexAttribArray(g_AttribLocationVtxPos);
//    glEnableVertexAttribArray(g_AttribLocationVtxUV);
//    glEnableVertexAttribArray(g_AttribLocationVtxColor);
//    glVertexAttribPointer(g_AttribLocationVtxPos,   2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
//    glVertexAttribPointer(g_AttribLocationVtxUV,    2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
//    glVertexAttribPointer(g_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));


}

void ImguiRender::render(CommandBuffer& buffer, const ImDrawData* draw_data)
{
    if (draw_data->DisplaySize.x <= 0 || draw_data->DisplaySize.x <= 0)
        return;

    const int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);


    // Will project scissor/clipping rectangles into framebuffer space
    const ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    setup_state(buffer, draw_data);

    Texture* last_bound_texture = nullptr;

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        // Upload vertex/index buffers
        m_geom->upload(
            cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size, m_vertex_format,
            cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size,
            IndexTypeTraits<ImDrawIdx>::index_type
        );

        SPDLOG_INFO("Uploading {} vertices, {} indices", cmd_list->VtxBuffer.Size, cmd_list->IdxBuffer.Size);

        buffer.bind_geometry(*m_geom, *m_shader);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    setup_state(buffer, draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Apply scissor/clipping rectangle
                    //glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
                    buffer.set_scissor({(int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y)});

                    // Bind texture, Draw
                    auto* texture = static_cast<Texture*>(pcmd->GetTexID());
                    if (texture) {
                        buffer.bind_texture(0, *texture);
                        last_bound_texture = texture;
                    } else {
                        ASSERT(last_bound_texture != nullptr);
                        buffer.unbind_texture(0, *last_bound_texture);
                        last_bound_texture = nullptr;
                    }
                    buffer.draw(PrimitiveType::Triangles, pcmd->IdxOffset, pcmd->ElemCount);
//                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
//                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
                }
            }
        }
    }

    if (last_bound_texture) {
        buffer.unbind_texture(0, *last_bound_texture);
    }
    buffer.set_blending_enabled(false);
    buffer.set_scissor_enabled(false);
    buffer.set_scissor({0, 0, fb_width, fb_height});

}


} // namespace Slic3r::App::Render