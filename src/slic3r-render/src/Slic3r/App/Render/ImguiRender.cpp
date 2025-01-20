#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Shader.hpp"
#include "Slic3r/App/Render/ShaderManager.hpp"
#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"

#include <Slic3r/Assert.hpp>
#include <libslic3r/Utils.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/fstream.hpp>

#include <imgui/imconfig.h>

// Following two sets keeps characters that ImGui tried to render, but they were not in the atlas,
// and ones that we already tried to add into the atlas.
std::set<ImWchar> s_missing_chars;
std::set<ImWchar> s_fixed_chars;
bool s_font_cjk{ false };

// This is a free function that ImGui calls when it renders
// a fallback glyph for c.
void imgui_rendered_fallback_glyph(ImWchar c)
{
    if (c == 0)
        return;

    if (ImGui::GetIO().Fonts->Fonts[0] == ImGui::GetFont()) {
        // Only do this when we are using the default ImGui font. Otherwise this would conflict with
        // EmbossStyleManager's font handling and we would load glyphs needlessly.
        auto it = s_fixed_chars.find(c);
        if (it == s_fixed_chars.end())
            // This is the first time we are trying to fix this character.
            s_missing_chars.emplace(c);
        else {
            // We already tried to add this, but it is still not there. There is a chance
            // that loading the CJK font would make this available.
            if (! s_font_cjk) {
                s_font_cjk = true;
                s_missing_chars.emplace(c);
                s_fixed_chars.erase(it);
            }
            else {
                // We did everything we could. The glyph was not available.
                // Do not try to add it anymore.
            }
        }
    }
}

namespace Slic3r::App::Render {

static const ImWchar ranges_latin2[] =
{
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0100, 0x017F, // Latin Extended-A
    0,
};
static const ImWchar ranges_turkish[] = {
	  0x0020, 0x01FF, // Basic Latin + Latin Supplement
	  0x0100, 0x017F, // Latin Extended-A
	  0x0180, 0x01FF, // Turkish
	  0,
};
static const ImWchar ranges_vietnamese[] =
{
    0x0020, 0x00FF, // Basic Latin
    0x0102, 0x0103,
    0x0110, 0x0111,
    0x0128, 0x0129,
    0x0168, 0x0169,
    0x01A0, 0x01A1,
    0x01AF, 0x01B0,
    0x1EA0, 0x1EF9,
    0,
};

#ifdef __APPLE__
static const ImWchar ranges_keyboard_shortcuts[] =
{
    0x21E7, 0x21E7, // OSX Shift Key symbol
    0x2318, 0x2318, // OSX Command Key symbol
    0x2325, 0x2325, // OSX Option Key symbol
    0,
};
#endif // __APPLE__

static const std::vector<std::pair<const wchar_t, std::string>> FONT_ICONS = {
    { ImGui::PrintIconMarker              , "cog"                            },
    { ImGui::PrinterIconMarker            , "printer"                        },
    { ImGui::PrinterSlaIconMarker         , "sla_printer"                    },
    { ImGui::FilamentIconMarker           , "spool"                          },
    { ImGui::MaterialIconMarker           , "resin"                          },
    { ImGui::MinimalizeButton             , "notification_minimalize"        },
    { ImGui::MinimalizeHoverButton        , "notification_minimalize_hover"  },
    { ImGui::RightArrowButton             , "notification_right"             },
    { ImGui::RightArrowHoverButton        , "notification_right_hover"       },
    { ImGui::PreferencesButton            , "notification_preferences"       },
    { ImGui::PreferencesHoverButton       , "notification_preferences_hover" },
    { ImGui::SliderFloatEditBtnIcon       , "edit_button"                    },
    { ImGui::SliderFloatEditBtnPressedIcon, "edit_button_pressed"            },
    { ImGui::ClipboardBtnIcon             , "copy_menu"                      },
    { ImGui::ExpandBtn                    , "expand_btn"                     },
    { ImGui::CollapseBtn                  , "collapse_btn"                   },
    { ImGui::RevertButton                 , "undo"                           },
    { ImGui::WarningMarkerSmall           , "notification_warning"           },
    { ImGui::InfoMarkerSmall              , "notification_info"              },
    { ImGui::PlugMarker                   , "plug"                           },
    { ImGui::DowelMarker                  , "dowel"                          },
    { ImGui::SnapMarker                   , "snap"                           },
    { ImGui::HorizontalHide               , "horizontal_hide"                },
    { ImGui::HorizontalShow               , "horizontal_show"                },
    { ImGui::PrintIdle                    , "print_idle"                     },
    { ImGui::PrintRunning                 , "print_running"                  },
    { ImGui::PrintFinished                , "print_finished"                 },
};

static const std::vector<std::pair<const wchar_t, std::string>> FONT_ICONS_MEDIUM = {
    { ImGui::Lock             , "lock_closed"       },
    { ImGui::LockHovered      , "lock_closed_f"     },
    { ImGui::Unlock           , "lock_open"         },
    { ImGui::UnlockHovered    , "lock_open_f"       },
    { ImGui::DSRevert         , "undo"              },
    { ImGui::DSRevertHovered  , "undo_f"            },
    { ImGui::DSSettings       , "cog"               },
    { ImGui::DSSettingsHovered, "cog_f"             },
    { ImGui::ErrorTick        , "error_tick"        },
    { ImGui::ErrorTickHovered , "error_tick_f"      },
    { ImGui::PausePrint       , "pause_print"       },
    { ImGui::PausePrintHovered, "pause_print_f"     },
    { ImGui::EditGCode        , "edit_gcode"        },
    { ImGui::EditGCodeHovered , "edit_gcode_f"      },
    { ImGui::RemoveTick       , "colorchange_del"   },
    { ImGui::RemoveTickHovered, "colorchange_del_f" },
};

static const std::vector<std::pair<const wchar_t, std::string>> FONT_ICONS_LARGE = {
    { ImGui::LegendTravel            , "legend_travel"                    },
    { ImGui::LegendWipe              , "legend_wipe"                      },
    { ImGui::LegendRetract           , "legend_retract"                   },
    { ImGui::LegendDeretract         , "legend_deretract"                 },
    { ImGui::LegendSeams             , "legend_seams"                     },
    { ImGui::LegendToolChanges       , "legend_toolchanges"               },
    { ImGui::LegendColorChanges      , "legend_colorchanges"              },
    { ImGui::LegendPausePrints       , "legend_pauseprints"               },
    { ImGui::LegendCustomGCodes      , "legend_customgcodes"              },
    { ImGui::LegendCOG               , "legend_cog"                       },
    { ImGui::LegendShells            , "legend_shells"                    },
    { ImGui::LegendToolMarker        , "legend_toolmarker"                },
    { ImGui::CloseNotifButton        , "notification_close"               },
    { ImGui::CloseNotifHoverButton   , "notification_close_hover"         },
    { ImGui::EjectButton             , "notification_eject_sd"            },
    { ImGui::EjectHoverButton        , "notification_eject_sd_hover"      },
    { ImGui::WarningMarker           , "notification_warning"             },
    { ImGui::ErrorMarker             , "notification_error"               },
    { ImGui::CancelButton            , "notification_cancel"              },
    { ImGui::CancelHoverButton       , "notification_cancel_hover"        },
//    { ImGui::SinkingObjectMarker     , "move"                             },
//    { ImGui::CustomSupportsMarker    , "fdm_supports"                     },
//    { ImGui::CustomSeamMarker        , "seam"                             },
//    { ImGui::MmuSegmentationMarker   , "mmu_segmentation"                 },
//    { ImGui::VarLayerHeightMarker    , "layers"                           },
    { ImGui::DocumentationButton     , "notification_documentation"       },
    { ImGui::DocumentationHoverButton, "notification_documentation_hover" },
    { ImGui::InfoMarker              , "notification_info"                },
    { ImGui::PlayButton              , "notification_play"                },
    { ImGui::PlayHoverButton         , "notification_play_hover"          },
    { ImGui::PauseButton             , "notification_pause"               },
    { ImGui::PauseHoverButton        , "notification_pause_hover"         },
    { ImGui::OpenButton              , "notification_open"                },
    { ImGui::OpenHoverButton         , "notification_open_hover"          },
    { ImGui::SlaViewOriginal         , "sla_view_original"                },
    { ImGui::SlaViewProcessed        , "sla_view_processed"               },
};
 
static const std::vector<std::pair<const wchar_t, std::string>> FONT_ICONS_EXTRA_LARGE = {
    { ImGui::ClippyMarker         , "notification_clippy"       },
    { ImGui::SliceAllBtnIcon      , "slice_all"                 },
    { ImGui::WarningMarkerDisabled, "notification_warning_grey" },
};

ImguiRender::ImguiRender(Device& device)
    : m_device(device)
    , m_vertex_format({
          {VertexAttribType::Vertex, DataType::Float, 2, IM_OFFSETOF(ImDrawVert, pos)},
          {VertexAttribType::TexCoord0, DataType::Float, 2, IM_OFFSETOF(ImDrawVert, uv)},
          {VertexAttribType::Color, DataType::UByte, 4, IM_OFFSETOF(ImDrawVert, col), true}
      })
{
    m_language_helper.lang_glyphs_info = {
        { "cs",   ranges_latin2, false },
        { "pl",   ranges_latin2, false },
        { "hu",   ranges_latin2, false },
        { "sl",   ranges_latin2, false },
        { "ru",   ImGui::GetIO().Fonts->GetGlyphRangesCyrillic(), false }, // Default + about 400 Cyrillic characters
        { "uk",   ImGui::GetIO().Fonts->GetGlyphRangesCyrillic(), false },
        { "be",   ImGui::GetIO().Fonts->GetGlyphRangesCyrillic(), false },
        { "tr",   ranges_turkish,    false },
        { "vi",   ranges_vietnamese, false },
        { "ja",   ImGui::GetIO().Fonts->GetGlyphRangesJapanese(), true }, // Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs
        { "ko",   ImGui::GetIO().Fonts->GetGlyphRangesKorean(),   true }, // Default + Korean characters
        { "zh_TW",ImGui::GetIO().Fonts->GetGlyphRangesChineseFull(), true }, // Traditional Chinese: Default + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
        { "zh",   ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon(), true }, // Simplified Chinese: Default + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
        { "th",   ImGui::GetIO().Fonts->GetGlyphRangesThai(),     false },
        { "else", ImGui::GetIO().Fonts->GetGlyphRangesDefault(),  false },
    };
}

void ImguiRender::set_font(const std::optional<std::string>& language, const std::optional<float>& font_size)
{
    if (!language.has_value() && !font_size.has_value())
        return;

    if (language.has_value())
        m_language_helper.language = *language;

    if (font_size.has_value()) {
        DEBUG_ASSERT(*font_size > 0.0f);
        m_language_helper.font_size = *font_size;
    }

    create_font_texture();
}

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
}

static void load_icon_from_svg(const std::pair<const wchar_t, std::string>& icon, int icon_sz, int rect_id, int tex_width,
    unsigned char* pixels) {
    ImGuiIO& io = ImGui::GetIO();
    if (const ImFontAtlas::CustomRect* rect = io.Fonts->GetCustomRectByIndex(rect_id)) {
        DEBUG_ASSERT(rect->Width == icon_sz);
        DEBUG_ASSERT(rect->Height == icon_sz);
        std::string filename = Slic3r::var(icon.second + ".svg");
        auto* codec = ImageCodecManager::instance().find_loader(filename);
        if (codec == nullptr) {
            SPDLOG_ERROR("Cannot find Image Reader Codec for file {}", filename);
            return;
        }
        boost::nowide::ifstream is(filename, std::ios::binary | std::ios::in);
        if (!is.good()) {
            SPDLOG_ERROR("Cannot open file {}", filename);
            return;
        }

        ImageLoadOptions opts;
        opts.max_size_px = icon_sz;
        std::vector<Image> images = codec->load(is, opts);
        if (images.empty()) {
            SPDLOG_ERROR("Cannot load image {}", filename);
            return;
        }

        const Image& image = images.front();
        DEBUG_ASSERT(image.width() <= icon_sz);
        DEBUG_ASSERT(image.height() <= icon_sz);

        const ImU32* pIn = (ImU32*)image.data();
        for (size_t y = 0; y < image.height(); ++y) {
            ImU32* pOut = (ImU32*)pixels + (rect->Y + y) * tex_width + rect->X;
            for (size_t x = 0; x < image.width(); ++x) {
                *pOut++ = *pIn++;
            }
        }
    }
}

void add_icons_rect_to_font_texture(ImguiLanguageHelper& language_helper, ImFont* font, int icon_sz, float font_scale)
{
    ImGuiIO& io = ImGui::GetIO();

    // add rectangles for the icons to the font atlas
    int px = icon_sz;
    for (auto& icon : FONT_ICONS) {
        language_helper.custom_glyph_rects_ids[icon.first] =
            io.Fonts->AddCustomRectFontGlyph(font, icon.first, px, px, 3.0 * font_scale + icon_sz);
    }

    px = int(1.25f * icon_sz);
    for (auto& icon : FONT_ICONS_MEDIUM) {
        language_helper.custom_glyph_rects_ids[icon.first] =
            io.Fonts->AddCustomRectFontGlyph(font, icon.first, px, px, 3.0 * font_scale + icon_sz);
    }

    px = 2 * icon_sz;
    for (auto& icon : FONT_ICONS_LARGE) {
        language_helper.custom_glyph_rects_ids[icon.first] =
            io.Fonts->AddCustomRectFontGlyph(font, icon.first, px, px, 3.0 * font_scale + icon_sz);
    }

    px = 4 * icon_sz;
    for (auto& icon : FONT_ICONS_EXTRA_LARGE) {
        language_helper.custom_glyph_rects_ids[icon.first] =
            io.Fonts->AddCustomRectFontGlyph(font, icon.first, px, px, 3.0 * font_scale + icon_sz);
    }
}

void load_icons_into_font_texture(int& rect_id, int icon_sz, int tex_width, unsigned char* pixels)
{
    // Fill rectangles from the SVG-icons
    int px = icon_sz;
    for (auto icon : FONT_ICONS) {
        load_icon_from_svg(icon, px, rect_id++, tex_width, pixels);
    }

    px = int(1.25f * icon_sz);
    for (auto icon : FONT_ICONS_MEDIUM) {
        load_icon_from_svg(icon, px, rect_id++, tex_width, pixels);
    }

    px = 2 * icon_sz;
    for (auto icon : FONT_ICONS_LARGE) {
        load_icon_from_svg(icon, px, rect_id++, tex_width, pixels);
    }

    px = 4 * icon_sz;
    for (auto icon : FONT_ICONS_EXTRA_LARGE) {
        load_icon_from_svg(icon, px, rect_id++, tex_width, pixels);
    }
}

void ImguiRender::create_font_texture()
{
    const ImWchar* glyph_ranges = nullptr;

    // Get glyph ranges for current language, std CLK flag to inform which font files need to be loaded.
    for (const auto& [lang_str, lang_ranges, lang_cjk] : m_language_helper.lang_glyphs_info) {
        if (boost::istarts_with(m_language_helper.language, lang_str) || lang_str == "else") {
            glyph_ranges = lang_ranges;
            s_font_cjk = lang_cjk;
            break;
        }
    }

    s_missing_chars.clear();
    s_fixed_chars.clear();

    if (glyph_ranges != m_language_helper.glyph_ranges) {
        m_language_helper.glyph_ranges = glyph_ranges;
        for (ImWchar c : s_fixed_chars) {
          s_missing_chars.emplace(c);
        }
        s_fixed_chars.clear();
    }

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Create ranges of characters from glyph_ranges, possibly adding some OS specific special characters.
    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(m_language_helper.glyph_ranges);

    builder.AddChar(ImWchar(0x2026)); // …

    if (s_font_cjk) {
        // https://github.com/prusa3d/PrusaSlicer/issues/8171: The translation
        // contains characters not in the ImGui ranges for simplified Chinese. Add them manually.
        // This should no longer be needed because the following block would add them automatically.
        builder.AddChar(ImWchar(0x5ED3));
        builder.AddChar(ImWchar(0x8F91));
    }

    // Add the characters that that needed the fallback character.
    for (ImWchar c : s_missing_chars) {
        builder.AddChar(c);
        s_fixed_chars.emplace(c);
    }
    s_missing_chars.clear();

#ifdef __APPLE__
  	if (s_font_cjk)
	    	// Apple keyboard shortcuts are only contained in the CJK fonts.
		    builder.AddRanges(ranges_keyboard_shortcuts);
#endif // __APPLE__

  	builder.BuildRanges(&ranges); // Build the final result (ordered ranges with all the unique characters submitted)

    ImFont* font = io.Fonts->AddFontFromFileTTF((Slic3r::resources_dir() + "/fonts/" + "NotoSans-Regular.ttf").c_str(), m_language_helper.font_size, nullptr, ranges.Data);
    if (s_font_cjk) {
        ImFontConfig config;
        config.MergeMode = true;
        io.Fonts->AddFontFromFileTTF((Slic3r::resources_dir() + "/fonts/" + "NotoSansCJK-Regular.ttc").c_str(), m_language_helper.font_size, &config, ranges.Data);
    }
    
    if (font == nullptr) {
        font = io.Fonts->AddFontDefault();
        if (font == nullptr)
            throw Slic3r::RuntimeError("ImGui: Could not load deafult font");
    }

    float font_scale = m_language_helper.font_size / 15.0f;
    int icon_sz = lround(16 * font_scale); // default size of icon is 16 px
    int rect_id = ImGui::GetIO().Fonts->CustomRects.Size;  // id of the rectangle added next
    add_icons_rect_to_font_texture(m_language_helper, font, icon_sz, font_scale);

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    load_icons_into_font_texture(rect_id, icon_sz, width, pixels);

    m_font_texture = m_device.context().texture_manager().create_empty("imgui_font", PixelFormat::RGBA8, width, height);
    m_font_texture->set_data(PixelFormat::RGBA8, 0, width, height, pixels);
//    m_textures[TextureType::Font]->set_filtering(Texture::MinFilter::Linear, Texture::MagFilter::Linear);
    io.Fonts->SetTexID(m_font_texture);
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
    buffer.set_depth_test_enabled(false);

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

    Matrix4f projection = ortho(left, right, bottom, top, -1, 1).cast<float>();
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

        //SPDLOG_DEBUG("Uploading {} vertices, {} indices", cmd_list->VtxBuffer.Size, cmd_list->IdxBuffer.Size);

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