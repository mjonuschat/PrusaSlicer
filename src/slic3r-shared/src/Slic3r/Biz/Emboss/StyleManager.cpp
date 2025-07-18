///|/ Copyright (c) Prusa Research 2022 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/Emboss/StyleManager.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include <optional>
#include <GL/glew.h> // Imgui texture
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
// #include <imgui/imgui_internal.h> // ImTextCharFromUtf8
#include <fast_float.h>

#include <boost/assign.hpp>
#include <boost/bimap.hpp>
#include <boost/log/trivial.hpp>

#include <libslic3r/Utils.hpp> // ScopeGuard

using namespace Slic3r;

/// <summary>
/// For store/load emboss style to/from file
/// </summary>
namespace {
constexpr std::uint32_t STYLE_OBJ_VERSION = 1;
using StylesObj                           = Biz::Emboss::StyleManager::StylesObj;
void store_styles_obj(const std::string& path, /*const */ StylesObj& data);
bool load_styles_obj(const std::string& path, StylesObj& data);
using Styles = Biz::Emboss::StyleManager::Styles;
void make_unique_name(const Styles& styles, std::string& name);
Styles create_default_styles(Biz::Emboss::IFontManager&);
} // namespace

namespace Slic3r::Biz::Emboss {
StyleManager::StyleManager(
    IFontManager& font_manager,
    const ImWchar* language_glyph_range,
    const ::std::string& cache_path
) :
    m_font_manager(font_manager),
    m_imgui_init_glyph_range(language_glyph_range),
    m_cache_path(cache_path)
{}

StyleManager::~StyleManager()
{
    clear_imgui_font();
    free_style_images();
}

void StyleManager::init()
{
    if (!load_styles_obj(m_cache_path, m_data)) {
        // No styles loaded from ini file so use default
        m_data.styles        = create_default_styles(m_font_manager);
        m_data.current_index = 0;
    }

    // find valid font item
    if (load_style(m_data.current_index))
        return; // style is loaded

    // Try to fix that style can't be loaded
    m_data.styles.erase(m_data.styles.begin() + m_data.current_index);
    load_valid_style();
}

bool StyleManager::store_styles_to_app_config(bool use_modification, bool store_active_index)
{
    if (m_data.styles.empty())
        return false;
    if (use_modification) {
        if (exist_stored_style()) {
            // update stored item
            m_data.styles[m_style_cache.style_index] = m_style_cache.style;
        } else {
            // add new into stored list
            Domain::EmbossStyle& style = m_style_cache.style.emboss_style;
            ::make_unique_name(m_data.styles, style.descriptor.name);
            m_style_cache.truncated_name.clear();
            m_style_cache.style_index = m_data.styles.size();
            m_data.styles.push_back({style});
        }
    }
    if (store_active_index && exist_stored_style()) {
        m_data.current_index = m_style_cache.style_index;
    }
    store_styles_obj(m_cache_path, m_data);
    return true;
}

void StyleManager::add_style(const std::string& name)
{
    Domain::EmbossStyle& style = m_style_cache.style.emboss_style;
    style.descriptor.name      = name;
    ::make_unique_name(m_data.styles, style.descriptor.name);
    m_style_cache.style_index = m_data.styles.size();
    m_style_cache.truncated_name.clear();
    m_data.styles.push_back({style});
}

void StyleManager::swap(size_t i1, size_t i2)
{
    if (i1 >= m_data.styles.size() || i2 >= m_data.styles.size())
        return;
    std::swap(m_data.styles[i1], m_data.styles[i2]);
    // fix selected index
    if (!exist_stored_style())
        return;
    if (m_style_cache.style_index == i1) {
        m_style_cache.style_index = i2;
    } else if (m_style_cache.style_index == i2) {
        m_style_cache.style_index = i1;
    }
}

void StyleManager::discard_style_changes()
{
    if (exist_stored_style()) {
        if (load_style(m_style_cache.style_index))
            return; // correct reload style
    } else {
        if (load_style(m_data.current_index))
            return; // correct load last used style
    }

    // try to save situation by load some font
    load_valid_style();
}

void StyleManager::erase(size_t index)
{
    if (index >= m_data.styles.size())
        return;

    // fix selected index
    if (exist_stored_style()) {
        size_t& i = m_style_cache.style_index;
        if (index < i)
            --i;
        else if (index == i)
            i = std::numeric_limits<size_t>::max();
    }

    m_data.styles.erase(m_data.styles.begin() + index);
}

void StyleManager::rename(const std::string& name)
{
    m_style_cache.style.emboss_style.descriptor.name = name;
    m_style_cache.truncated_name.clear();
    if (exist_stored_style()) {
        Style& it                       = m_data.styles[m_style_cache.style_index];
        it.emboss_style.descriptor.name = name;
        it.truncated_name.clear();
    }
}

void StyleManager::load_valid_style()
{
    // iterate over all known styles
    while (!m_data.styles.empty()) {
        if (load_style(0))
            return;
        // can't load so erase it from list
        m_data.styles.erase(m_data.styles.begin());
    }

    // no one style is loadable
    // set up default font list
    m_data.styles        = create_default_styles(m_font_manager);
    m_data.current_index = 0;

    // iterate over default styles
    // There have to be option to use build in font
    while (!m_data.styles.empty()) {
        if (load_style(0))
            return;
        // can't load so erase it from list
        m_data.styles.erase(m_data.styles.begin());
    }

    // This OS doesn't have TTF as default font,
    // find some loadable font out of default list
    assert(false);
}

bool StyleManager::load_style(size_t style_index)
{
    if (style_index >= m_data.styles.size())
        return false;
    if (!load_style(m_data.styles[style_index]))
        return false;
    m_style_cache.style_index = style_index;
    m_data.current_index      = style_index;
    return true;
}

bool StyleManager::load_style(const Style& style)
{
    if (style.emboss_style.descriptor.type == Domain::FontDescriptor::Type::file_path) {
        std::unique_ptr<Domain::FontFile> font_ptr = create_font_file(
            style.emboss_style.descriptor.path.c_str()
        );
        if (font_ptr == nullptr)
            return false;
        m_style_cache.font_file   = FontFileWithCache(std::move(font_ptr));
        m_style_cache.style       = style; // copy
        m_style_cache.style_index = std::numeric_limits<size_t>::max();
        return true;
    }

    m_style_cache.style       = style; // copy
    m_style_cache.style_index = std::numeric_limits<size_t>::max();
    m_style_cache.truncated_name.clear();

    return true;
}

bool StyleManager::is_font_changed() const
{
    if (!exist_stored_style())
        return false;
    const Style* stored_style = get_stored_style();
    if (stored_style == nullptr)
        return false;

    const Domain::FontProp& prop        = get_style().emboss_style.prop;
    const Domain::FontProp& prop_stored = stored_style->emboss_style.prop;

    // Exist change in face name?
    // if(wx_font_stored.GetFaceName() != wx_font.GetFaceName()) return true;

    const std::optional<float>& skew = prop.skew;
    bool is_italic                   = skew.has_value(); // || WxFontUtils::is_italic(wx_font);
    const std::optional<float>& skew_stored = prop_stored.skew;
    bool is_stored_italic = skew_stored.has_value(); // || WxFontUtils::is_italic(wx_font_stored);
    // is italic changed
    if (is_italic != is_stored_italic)
        return true;

    const std::optional<float>& boldness = prop.boldness;
    bool is_bold = boldness.has_value(); // || WxFontUtils::is_bold(wx_font);
    const std::optional<float>& boldness_stored = prop_stored.boldness;
    bool is_stored_bold = boldness_stored.has_value(); // || WxFontUtils::is_bold(wx_font_stored);
    // is bold changed
    return is_bold != is_stored_bold;
}

bool StyleManager::is_unique_style_name(const std::string& name) const
{
    for (const StyleManager::Style& style : m_data.styles)
        if (style.emboss_style.descriptor.name == name)
            return false;
    return true;
}

bool StyleManager::is_active_font()
{
    return m_style_cache.font_file.has_value();
}

const StyleManager::Style* StyleManager::get_stored_style() const
{
    if (m_style_cache.style_index >= m_data.styles.size())
        return nullptr;
    return &m_data.styles[m_style_cache.style_index];
}

void StyleManager::clear_glyphs_cache()
{
    FontFileWithCache& ff = m_style_cache.font_file;
    if (!ff.has_value())
        return;
    ff.cache = std::make_shared<Glyphs>();
}

void StyleManager::clear_imgui_font()
{
    m_style_cache.atlas.Clear();
}

ImFont* StyleManager::get_imgui_font()
{
    if (!is_active_font())
        return nullptr;

    ImVector<ImFont*>& fonts = m_style_cache.atlas.Fonts;
    if (fonts.empty())
        return nullptr;

    // check correct index
    int f_size = fonts.size();
    assert(f_size == 1);
    if (f_size != 1)
        return nullptr;
    ImFont* font = fonts.front();
    if (font == nullptr)
        return nullptr;
    return font;
}

const StyleManager::Styles& StyleManager::get_styles() const
{
    return m_data.styles;
}

std::vector<std::string> StyleManager::get_style_names() const 
{
    std::vector<std::string> names;
    names.reserve(m_data.styles.size());
    for (const Biz::Emboss::StyleManager::Style& style : m_data.styles)
        names.push_back(style.emboss_style.descriptor.name);
    return names;
}

void StyleManager::init_trunc_names(float max_width)
{
    for (auto& s : m_data.styles)
        if (s.truncated_name.empty()) {
            std::string name = s.emboss_style.descriptor.name;
            App::Imgui::escape_double_hash(name);
            s.truncated_name = App::Imgui::trunc(name, max_width);
        }
}

void StyleManager::init_style_images(const Domain::Index2& max_size, const std::string& text)
{
    // check already initialized
    if (m_exist_style_images)
        return;

    // check is initializing
    if (m_temp_style_images != nullptr) {
        // is initialization finished
        if (!m_temp_style_images->styles.empty()) {
            assert(m_temp_style_images->images.size() == m_temp_style_images->styles.size());
            // copy images into styles
            for (StyleManager::StyleImage& image : m_temp_style_images->images) {
                size_t index                 = &image - &m_temp_style_images->images.front();
                StyleImagesData::Item& style = m_temp_style_images->styles[index];

                // find style in font list and copy to it
                for (auto& it : m_data.styles) {
                    if (it.emboss_style.descriptor.name != style.text
                        || !(it.emboss_style.prop == style.prop))
                        continue;
                    it.image = image;
                    break;
                }
            }
            m_temp_style_images  = nullptr;
            m_exist_style_images = true;
            return;
        }
        // in process of initialization inside of job
        return;
    }

    // create job for init images
    m_temp_style_images = std::make_shared<StyleImagesData::StyleImages>();
    StyleImagesData::Items styles;
    styles.reserve(m_data.styles.size());
    for (const Style& style : m_data.styles) {
        std::unique_ptr<const Domain::FontFile> font_file = m_font_manager.open(
            style.emboss_style.descriptor
        );
        if (font_file == nullptr)
            continue;
        styles.push_back(
            {FontFileWithCache(std::move(font_file)),
             style.emboss_style.descriptor.name,
             style.emboss_style.prop}
        );
    }

    // TODO: Implement it
    // auto mf = wxGetApp().mainframe;
    //// dot per inch for monitor
    // int dpi = get_dpi_for_window(mf);
    //// pixel per milimeter
    // double ppm = dpi / ObjectManipulation::in_to_mm;

    // auto &worker = wxGetApp().plater()->get_ui_job_worker();
    // StyleImagesData data{std::move(styles), max_size, text, m_temp_style_images, ppm};
    // queue_job(worker, std::make_unique<CreateFontStyleImagesJob>(std::move(data)));
}

void StyleManager::free_style_images()
{
    if (!m_exist_style_images)
        return;
    GLuint tex_id = 0;
    for (Style& it : m_data.styles) {
        if (tex_id == 0 && it.image.has_value())
            tex_id = (GLuint) (intptr_t) it.image->texture_id;
        it.image.reset();
    }
    // if (tex_id != 0)
    // glsafe(::glDeleteTextures(1, &tex_id));
    m_exist_style_images = false;
}

float StyleManager::min_imgui_font_size = 18.f;
float StyleManager::max_imgui_font_size = 60.f;

float StyleManager::get_imgui_font_size(
    const Domain::FontProp& prop,
    const Domain::FontFile& file,
    double scale
)
{
    const Domain::FontFile::Info& info = get_font_info(file, prop);
    // coeficient for convert line height to font size
    float c1 = (info.ascent - info.descent + info.linegap) / (float) info.unit_per_em;

    // The point size is defined as 1/72 of the Anglo-Saxon inch (25.4 mm):
    // It is approximately 0.0139 inch or 352.8 um.
    return c1 * std::abs(prop.size_in_mm) / 0.3528f * scale;
}

ImFont* StyleManager::create_imgui_font(const std::string& text, double scale)
{
    // inspiration inside of ImGuiWrapper::init_font
    auto& ff = m_style_cache.font_file;
    if (!ff.has_value())
        return nullptr;
    const Domain::FontFile& font_file = *ff.font_file;

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(m_imgui_init_glyph_range);
    if (!text.empty())
        builder.AddText(text.c_str());

    ImVector<ImWchar>& ranges = m_style_cache.ranges;
    ranges.clear();
    builder.BuildRanges(&ranges);

    m_style_cache.atlas.Flags |= ImFontAtlasFlags_NoMouseCursors
        | ImFontAtlasFlags_NoPowerOfTwoHeight;

    const Domain::FontProp& font_prop = m_style_cache.style.emboss_style.prop;
    float font_size                   = get_imgui_font_size(font_prop, font_file, scale);
    if (font_size < min_imgui_font_size)
        font_size = min_imgui_font_size;
    if (font_size > max_imgui_font_size)
        font_size = max_imgui_font_size;

    ImFontConfig font_config;
    // TODO: start using merge mode
    // font_config.MergeMode = true;
    int unit_per_em = get_font_info(font_file, font_prop).unit_per_em;
    float coef      = font_size / (double) unit_per_em;
    if (font_prop.char_gap.has_value())
        font_config.GlyphExtraSpacing.x = coef * (*font_prop.char_gap);
    if (font_prop.line_gap.has_value())
        font_config.GlyphExtraSpacing.y = coef * (*font_prop.line_gap);

    font_config.FontDataOwnedByAtlas = false;

    const std::vector<unsigned char>& buffer = *font_file.data;
    ImFont* font                             = m_style_cache.atlas.AddFontFromMemoryTTF(
        (void*) buffer.data(),
        buffer.size(),
        font_size,
        &font_config,
        m_style_cache.ranges.Data
    );

    unsigned char* pixels;
    int width, height;
    m_style_cache.atlas.GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;

    ////////////////// TODO: solve storing texture
    // glsafe(::glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture));
    // ScopeGuard sg([last_texture]() {
    // glsafe(::glBindTexture(GL_TEXTURE_2D, last_texture));
    //});

    GLuint font_texture;
    // glsafe(::glGenTextures(1, &font_texture));
    // glsafe(::glBindTexture(GL_TEXTURE_2D, font_texture));
    // glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    // glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    // glsafe(::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    // if (OpenGLManager::are_compressed_textures_supported())
    // glsafe(::glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
    // else
    // glsafe(::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));

    // Store our identifier
    m_style_cache.atlas.TexID = (ImTextureID) (intptr_t) font_texture;
    assert(!m_style_cache.atlas.Fonts.empty());
    if (m_style_cache.atlas.Fonts.empty())
        return nullptr;
    assert(font == m_style_cache.atlas.Fonts.back());
    if (!font->IsLoaded())
        return nullptr;
    assert(font->IsLoaded());
    return font;
}

} // namespace Slic3r::Biz::Emboss

#include <fstream>
// cache font list by cereal
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/archives/binary.hpp>

namespace cereal {
template <class Archive>
void serialize(Archive& ar, Biz::Emboss::StyleManager::Style& s)
{
    // ignore truncated_name and image(which are created on demand)
    ar((Domain::EmbossStyle&) s, s.projection, s.distance, s.angle);
}

template <class Archive>
void serialize(Archive& ar, Biz::Emboss::StyleManager::StylesObj& data, const std::uint32_t version)
{
    // When performing a load, the version associated with the class
    // is whatever it was when that data was originally serialized
    // When we save, we'll use the version that is defined in the macro
    if (version != ::STYLE_OBJ_VERSION)
        return;
    ar(data.styles, data.current_index);
}
} // namespace cereal

// StylesSerializable
namespace {
void store_styles_obj(const std::string& path, StylesObj& data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(error)
            << "Text Preset can't be stored. "
            << "Failed to open cache file "
            << path
            << " for writing.";
        return;
    }
    cereal::BinaryOutputArchive archive(file);
    try {
        archive(data);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store file - " << path << ": " << ex.what();
    }
}

bool load_styles_obj(const std::string& path, StylesObj& data)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) // Cache File not found (not created yet)
        return false;
    cereal::BinaryInputArchive archive(file);
    try {
        archive(data);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Failed to read from file - " << path << ": " << ex.what();
        return false;
    }
    return true;
}

void make_unique_name(const Biz::Emboss::StyleManager::Styles& styles, std::string& name)
{
    auto is_unique = [&styles](const std::string& name) {
        for (const Biz::Emboss::StyleManager::Style& it : styles)
            if (it.emboss_style.descriptor.name == name)
                return false;
        return true;
    };

    // Style name can't be empty so default name is set
    if (name.empty())
        name = "Text style";

    // When name is already unique, nothing need to be changed
    if (is_unique(name))
        return;

    // when there is previous version of style name only find number
    const char* prefix = " (";
    const char suffix  = ')';
    auto pos           = name.find_last_of(prefix);
    if (name.c_str()[name.size() - 1] == suffix && pos != std::string::npos) {
        // short name by ord number
        name = name.substr(0, pos);
    }

    int order = 1; // start with value 2 to represents same font name
    std::string new_name;
    do {
        new_name = name + prefix + std::to_string(++order) + suffix;
    } while (!is_unique(new_name));
    name = new_name;
}

Styles create_default_styles(Biz::Emboss::IFontManager& font_manager)
{
    Styles styles;
    Domain::FontList favorits = font_manager.create_favorit();
    for (Domain::FontDescriptor& favorit : favorits) {
        ::make_unique_name(styles, favorit.name);
        styles.push_back(
            Biz::Emboss::StyleManager::Style{.emboss_style = Domain::EmbossStyle{.descriptor = favorit}}
        );
    }
    return styles;
}

} // namespace

CEREAL_CLASS_VERSION(Slic3r::Biz::Emboss::StyleManager::StylesObj, ::STYLE_OBJ_VERSION); // register class version
