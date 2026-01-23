#include "Slic3r/App/Render/ImguiFontHelper.hpp"

#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Exception.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/ImguiIconHelper.hpp"


#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>
#include "Slic3r/Directories.hpp"

#include <imgui/imgui.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/fstream.hpp>

#include <set>
#include <unordered_set>

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

static const std::unordered_set<Icon> FONT_ICONS = {
    // Icons used by ObjectList
    Icon::PrinterIconMarker            ,
    Icon::PrinterSlaIconMarker         ,
    Icon::SliderFloatEditBtnIcon       ,
    Icon::SliderFloatEditBtnPressedIcon,
    Icon::PlugMarker                   ,
    Icon::DowelMarker                  ,
    Icon::SnapMarker                   ,
    Icon::EyeOpen                      ,
    Icon::EyeClosed                    ,
    Icon::SolidPartVolume              ,
    Icon::NegativeVolume               ,
    Icon::ModifierVolume               ,
    Icon::SupportBlocker               ,
    Icon::SupportModifier              ,
    Icon::TextSolidPartVolume          ,
    Icon::TextNegativeVolume           ,
    Icon::TextModifierVolume           ,
    Icon::SvgSolidPartVolume           ,
    Icon::SvgNegativeVolume            ,
    Icon::SvgModifierVolume            ,
    Icon::ObjectIcon                   ,
    Icon::HRModifier                   ,
    Icon::CustomSupports               ,
    Icon::CustomSeam                   ,
    Icon::CutConnectors                ,
    Icon::MmSegmentation               ,
    Icon::Sinking                      ,
    Icon::FuzzySkin                    ,
    Icon::OpenArrow                    ,
    Icon::CloseArrow                   ,
    Icon::ConfigContainer              ,
    Icon::InstancesIcon                ,
    Icon::AddBedIcon                   ,
    Icon::DelBedIcon                   ,
    Icon::OverridesMarker              ,
    Icon::AllBeds                      ,
    Icon::Palette                      ,
};

static const std::unordered_set<Icon> FONT_ICONS_MEDIUM = {
    // double slider icons
    Icon::ErrorTick        ,
    Icon::ErrorTickHovered ,
    Icon::PausePrint       ,
    Icon::PausePrintHovered,
    Icon::EditGCode        ,
    Icon::EditGCodeHovered ,
    Icon::RemoveTick       ,
    Icon::RemoveTickHovered,
};

ImguiFontHelper::ImguiFontHelper(Device& device)
  : m_device(device)
{
    m_language_data.lang_glyphs_info = {
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

void ImguiFontHelper::set_font(const std::optional<std::string>& language, const std::optional<float>& font_size,
    const std::optional<float>& font_global_scale)
{
    if (!language.has_value() && !font_size.has_value())
        return;

    if (language.has_value())
        m_language_data.language = *language;

    if (font_size.has_value()) {
        DEBUG_ASSERT(*font_size > 0.0f);
        m_language_data.font_size = *font_size;
    }

    create_font_texture();

    if (font_global_scale.has_value())
        ImGui::GetIO().FontGlobalScale = 1.0f / *font_global_scale;
}

ImFont* ImguiFontHelper::font(Render::ImguiFontType type)
{
    auto it = m_fonts.find(type);
    DEBUG_ASSERT(it != m_fonts.end());
    return (it != m_fonts.end()) ? it->second : nullptr;
}

static void add_icons_rect_to_font_texture(const ImguiFontHelper& helper, ImguiLanguageData& language_data, ImFont* font)
{
    ImGuiIO& io = ImGui::GetIO();

    float advance = helper.icon_advance();

    auto add_icons = [font, &language_data, &io, advance](const int px, const std::unordered_set<Icon>& icons) {
        for (const Icon icon : icons) {
            wchar_t icon_char = static_cast<wchar_t>(icon);
            language_data.custom_glyph_rects_ids[icon_char] =
                io.Fonts->AddCustomRectFontGlyph(font, icon_char, px, px, advance + px);
        }
    };
 
    // add rectangles for the icons to the font atlas
    add_icons(helper.icon_size(), FONT_ICONS);
    add_icons(helper.icon_medium_size(), FONT_ICONS_MEDIUM);
}

static void load_icon_from_file(const std::string& icon_name, int icon_sz, int rect_id, int tex_width,
    unsigned char* pixels) {
    ImGuiIO& io = ImGui::GetIO();
    if (const ImFontAtlasCustomRect* rect = io.Fonts->GetCustomRectByIndex(rect_id)) {
        DEBUG_ASSERT(rect->Width == icon_sz);
        DEBUG_ASSERT(rect->Height == icon_sz);

        std::string filename = Slic3r::resources_dir() + "/" + icon_name;
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
        std::vector<Domain::Image> images = codec->load(is, opts);
        if (images.empty()) {
            SPDLOG_ERROR("Cannot load image {}", filename);
            return;
        }

        const Domain::Image& image = images.front();
        DEBUG_ASSERT(image.width() <= icon_sz);
        DEBUG_ASSERT(image.height() <= icon_sz);

        const ImU32* pIn = (ImU32*)image.pixels.data();
        for (size_t y = 0; y < image.height(); ++y) {
            ImU32* pOut = (ImU32*)pixels + (rect->Y + y) * tex_width + rect->X;
            for (size_t x = 0; x < image.width(); ++x) {
                *pOut++ = *pIn++;
            }
        }
    }
}

static void load_icons_into_font_texture(const ImguiFontHelper& helper, int& rect_id, int tex_width, unsigned char* pixels)
{
    auto load_icons = [&](const int px, const std::unordered_set<Icon>& icons) {
        for (const Icon icon : icons) {
            load_icon_from_file(ImguiIconHelper::icon_path(icon), px, rect_id++, tex_width, pixels);
        }
    };

    // Fill rectangles from the SVG-icons
    load_icons(helper.icon_size(), FONT_ICONS);
    load_icons(helper.icon_medium_size(), FONT_ICONS_MEDIUM);
}

static ImFont* load_font(const std::string& filename, const std::string& filename_cjk, const ImguiLanguageData& language_data, const ImVector<ImWchar>& ranges,
    bool font_cjk)
{
    ImGuiIO& io = ImGui::GetIO();
    std::string path = Slic3r::resources_dir() + "/fonts/" + filename;
    std::string path_cjk = Slic3r::resources_dir() + "/fonts/" + filename_cjk;
    ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), language_data.font_size, nullptr, ranges.Data);
    if (font_cjk) {
        ImFontConfig config;
        config.MergeMode = true;
        io.Fonts->AddFontFromFileTTF(path_cjk.c_str(), language_data.font_size, &config, ranges.Data);
    }

    if (font == nullptr) {
        font = io.Fonts->AddFontDefault();
        if (font == nullptr)
            throw Slic3r::RuntimeError("ImGui: Could not load default font");
    }

    return font;
}

void ImguiFontHelper::create_font_texture()
{
    const ImWchar* glyph_ranges = nullptr;

    bool font_cjk{false};

    // Get glyph ranges for current language, std CLK flag to inform which font files need to be loaded.
    for (const auto& [lang_str, lang_ranges, lang_cjk] : m_language_data.lang_glyphs_info) {
        if (boost::istarts_with(m_language_data.language, lang_str) || lang_str == "else") {
            glyph_ranges = lang_ranges;
            font_cjk = lang_cjk;
            break;
        }
    }

    if (glyph_ranges != m_language_data.glyph_ranges) {
        m_language_data.glyph_ranges = glyph_ranges;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Create ranges of characters from glyph_ranges, possibly adding some OS specific special characters.
    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(m_language_data.glyph_ranges);

    builder.AddChar(ImWchar(0x2026)); // …

    if (font_cjk) {
        // https://github.com/prusa3d/PrusaSlicer/issues/8171: The translation
        // contains characters not in the ImGui ranges for simplified Chinese. Add them manually.
        // This should no longer be needed because the following block would add them automatically.
        builder.AddChar(ImWchar(0x5ED3));
        builder.AddChar(ImWchar(0x8F91));
    }

#ifdef __APPLE__
  	if (font_cjk)
	    	// Apple keyboard shortcuts are only contained in the CJK fonts.
		    builder.AddRanges(ranges_keyboard_shortcuts);
#endif // __APPLE__

  	builder.BuildRanges(&ranges); // Build the final result (ordered ranges with all the unique characters submitted)

    m_fonts[ImguiFontType::Regular] = load_font("NotoSans-Regular.ttf", "NotoSansCJK-Regular.ttc", m_language_data, ranges, font_cjk);
    m_fonts[ImguiFontType::Bold]    = load_font("NotoSans-Bold.ttf", "NotoSansCJK-Bold.ttc", m_language_data, ranges, font_cjk);
    m_fonts[ImguiFontType::Italic]  = load_font("NotoSans-Italic.ttf", "NotoSansCJK-Italic.ttc", m_language_data, ranges, font_cjk);

    int rect_id = ImGui::GetIO().Fonts->CustomRects.Size;  // id of the rectangle added next
    for (auto& [type, font] : m_fonts) {
        add_icons_rect_to_font_texture(*this, m_language_data, font);
    }

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Load icons for all fonts textures
    for (auto& f : m_fonts) {
        load_icons_into_font_texture(*this, rect_id, width, pixels);
    }

    m_font_texture = m_device.context().texture_manager().get_or_create_dynamic("imgui_font", Domain::PixelFormat::RGBA8, width, height);
    m_font_texture->set_data(Domain::PixelFormat::RGBA8, 0, width, height, pixels, size_t(4 * width * height));
//    m_textures[TextureType::Font]->set_filtering(Texture::MinFilter::Linear, Texture::MagFilter::Linear);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)m_font_texture.get());
}

} // namespace Slic3r::App::Render
