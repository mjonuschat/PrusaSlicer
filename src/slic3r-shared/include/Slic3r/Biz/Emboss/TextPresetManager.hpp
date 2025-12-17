///|/ Copyright (c) Prusa Research 2022 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_EmbossTextPresetManager_hpp_
#define slic3r_EmbossTextPresetManager_hpp_

#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <imgui/imgui.h>

#include "Slic3r/Domain/FontFile.hpp"
#include "Slic3r/Biz/Emboss/Emboss.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Domain/EmbossShape.hpp"

namespace Slic3r::Biz::Emboss {
/**
@brief Manage Emboss text preset(style of the embossing)
Cache current state of preset
*/
class TextPresetManager
{
public:
    /**
    @param font_manager Accessor to font file data via Domain::FontDescriptor
    @param language_glyph_range Character to load for imgui when initialize imgui font
    @param cache_path File path for store cache with current user Emboss presets.
        @note data_dir() + "/emboss_presets.cereal"
    @param project_interactor For initialize ProjectScoped data only
    */
    TextPresetManager(
        IFontManager& font_manager,
        const ImWchar* language_glyph_range,
        const std::string& cache_path,
        Biz::ProjectInteractor& project_interactor
    );

    /**
    @brief Load font presets from file,
    Also select actual activ font
    */
    void init();

    /**
    @brief Write font list into AppConfig
    @param use_modification When true cache state will be used for store
    @param store_active_index When treu also store current activ index
    @return True on succes otherwise False.
    */
    bool store_presets(bool use_modification = true, bool store_active_index = true);

    void save_preset_as();
    void rename_preset();
    bool delete_preset();

    /**
    @brief Discard changes in current preset
    When no activ preset use last used OR first loadable
    */
    void discard_preset_changes();

    /**
    @brief load some valid preset
    */
    void load_valid_preset();

    /**
    @brief Change current preset
    When font can't load, roll back current preset
    @param preset_index New preset index(from m_presets range)
    @return True on succes. False on fail load font
    */
    bool load_preset(size_t preset_index);
    // load font preset not stored in list
    struct Preset;
    bool load_preset(const Preset& preset);

    // clear actual selected glyphs cache
    void clear_glyphs_cache();

    // remove cached imgui font for actual selected font
    void clear_imgui_font();

    // setter for font
    void set_font(const Domain::FontDescriptor& font_descriptor);

    // getters for private data
    const Preset* get_stored_preset() const;

    const Preset& get_preset() const
    {
        return m_proj_preset_cache.selected().preset;
    }

    Preset& get_preset()
    {
        return m_proj_preset_cache.selected().preset;
    }
    
    // For fill select box in dialog, used together with get_preset_index() 
    std::vector<std::string> get_presets_names() const;
    size_t get_preset_index() const
    {
        return m_proj_preset_cache.selected().preset_index;
    }

    const Domain::FontProp& get_font_prop() const
    {
        return get_preset().emboss_style.prop;
    }

    Domain::FontProp& get_font_prop()
    {
        return get_preset().emboss_style.prop;
    }

    FontFileWithCache& get_font_file_with_cache()
    {
        PresetCache& cache = m_proj_preset_cache.selected();
        FontFileWithCache& ff = cache.font_file;
        if (ff.has_value())
            return ff; // use cache
        // create new cache
        ff = FontFileWithCache(m_font_manager.open(cache.preset.emboss_style.descriptor));
        return ff; 
    }

    bool has_collections() const
    {
        const FontFileWithCache& ff = m_proj_preset_cache.selected().font_file;
        return ff.has_value()
            && ff.font_file->infos.size() > 1;
    }

    // True when activ style has same name as some of stored style
    bool exist_stored_style() const
    {
        return m_proj_preset_cache.selected().preset_index != std::numeric_limits<size_t>::max();
    }

    /**
    @brief check whether current style differ to selected
    */
    bool is_font_changed() const;

    bool is_unique_style_name(const std::string& name) const;

    // Getter on acitve font pointer for imgui
    // Initialize imgui font(generate texture) when doesn't exist yet.
    // Extend font atlas when not in glyph range
    ImFont* get_imgui_font();
    // initialize font range by unique symbols in text
    ImFont* create_imgui_font(const std::string& text, double scale);

    /**
    @brief Initialization texture with rendered font style
    @param max_size Maximal width and height of one style texture
    @param text Text to render by style
    */
    void init_style_images(const Domain::Index2& max_size, const std::string& text);
    void free_style_images();

    // access to all managed font styles
    const std::vector<Preset>& get_presets() const;


    /**
    @brief Describe image in GPU to show settings of style
    */
    struct PresetImage
    {
        void* texture_id = nullptr; // GLuint
        Domain::BoundingBox<int, 2> bounding_box;
        ImVec2 tex_size;
        ImVec2 uv0;
        ImVec2 uv1;
        Domain::Point offset = Domain::Point(0, 0);
    };

    /**
    @brief All connected with one style
    keep temporary data and caches for style
    */
    struct Preset
    {
        Domain::EmbossStyle emboss_style;

        // Define how to emboss shape
        Domain::EmbossProjection projection;

        // distance from surface point
        // used for move over model surface
        // When not set value is zero and is not stored
        std::optional<float> distance; // [in mm]

        // Angle of rotation around emboss direction (Z axis)
        // It is calculate on the fly from volume world transformation
        // only TextPresetManager keep actual value for comparision with style
        // When not set value is zero and is not stored
        std::optional<float> angle; // [in radians] form -Pi to Pi

        bool operator==(const Preset& other) const
        {
            return emboss_style == other.emboss_style
                && projection == other.projection
                && Domain::is_approx(distance, other.distance)
                && Domain::is_approx(angle, other.angle);
        }

        // visualization of style
        std::optional<PresetImage> image;
    };

    using Presets = std::vector<Preset>;

    struct PresetsObj
    {
        Presets presets;
        size_t current_index;
    };

    // Limits for imgui loaded font size
    // Value out of limits is crop
    static float min_imgui_font_size;
    static float max_imgui_font_size;
    static float get_imgui_font_size(
        const Domain::FontProp& prop,
        const Domain::FontFile& file,
        double scale
    );

private:
    IFontManager& m_font_manager;
    // keep language dependent glyph range
    const ImWchar* m_imgui_init_glyph_range;
    std::string m_cache_path;

    /**
    @brief Cache data from style to reduce amount of:
    1) loading font from file
    2) Create atlas of symbols for imgui
    3) Keep loaded(and modified by style) glyphs from font
    */
    struct PresetCache
    {
        // share font file data with emboss job thread
        FontFileWithCache font_file = {};

        // must live same as imgui_font inside of atlas
        ImVector<ImWchar> ranges = {};

        // Keep only actual style in atlas
        ImFontAtlas atlas = {};

        // cache for view font name with maximal width in imgui
        std::string truncated_name;

        // actual used font item
        Preset preset = {};

        // index into m_presets
        size_t preset_index = std::numeric_limits<size_t>::max();
    };

    Biz::ProjectScoped<PresetCache> m_proj_preset_cache;

    // Privat member
    PresetsObj m_data;

    /**
    @brief Keep data needed to create Font Preset Images in Job
    */
    struct PresetImagesData
    {
        struct Item
        {
            FontFileWithCache font;
            std::string text;
            Domain::FontProp prop;
        };

        using Items = std::vector<Item>;

        // Keep styles to render
        Items styles;
        // Maximal width and height in pixels of image
        Domain::Index2 max_size;
        // Text to render
        std::string text;

        /**
        @brief Result of job
        */
        struct PresetImages
        {
            // vector of inputs
            PresetImagesData::Items presets;
            // job output
            std::vector<PresetImage> images;
        };

        // place to store result in main thread in Finalize
        std::shared_ptr<PresetImages> result;

        // pixel per milimeter (scaled DPI)
        double ppm;
    };

    std::shared_ptr<PresetImagesData::PresetImages> m_temp_style_images = nullptr;
    bool m_exist_style_images                                         = false;
};

} // namespace Slic3r::Biz::Emboss

#endif // slic3r_EmbossTextPresetManager_hpp_
