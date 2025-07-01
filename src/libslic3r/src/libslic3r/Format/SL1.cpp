///|/ Copyright (c) Prusa Research 2020 - 2023 Tomáš Mészáros @tamasmeszaros, Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "SL1.hpp"

#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include <sstream>

#include "Slic3r/Time.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Zipper.hpp"

#include "libslic3r/miniz_extension.hpp" // IWYU pragma: keep
#include <LocalesUtils.hpp>
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "libslic3r/Utils/JsonUtils.hpp"

#include "libslic3r/SLA/RasterBase.hpp"


#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

using namespace Slic3r::Biz::Slicing;

namespace Slic3r {

using ConfMap = std::map<std::string, std::string>;
using Domain::EnumVectorWrapper;

namespace {

std::string to_ini(const ConfMap &m)
{
    std::string ret;
    for (auto &param : m)
        ret += param.first + " = " + param.second + "\n";

    return ret;
}

const std::vector<std::string> ms_opts{
    "delay_before_exposure",
    "delay_after_exposure",
    "tilt_down_offset_delay",
    "tilt_up_offset_delay",
    "tilt_down_delay",
    "tilt_up_delay",
};

const std::string tower_hop_height_opt{
   "tower_hop_height"
};

const std::vector<std::string> speed_opts{
    "tower_speed",
    "tilt_down_initial_speed",
    "tilt_down_finish_speed",
    "tilt_up_initial_speed",
    "tilt_up_finish_speed",
};

const std::string use_tilt_opt{"use_tilt"};

const std::vector<std::string> count_opts{
    "tilt_down_offset_steps",
    "tilt_down_cycles",
    "tilt_up_offset_steps",
    "tilt_up_cycles"
};

namespace pt = boost::property_tree;

std::string tilt_options_to_json(const SLAPrintConfigView& cfg, const ConfMap& iniconf)
{
    pt::ptree below_node;
    pt::ptree above_node;

    for (const std::string& key : ms_opts) {
        const auto values{cfg.get<std::vector<double>>(key)};
        const double coeff{1e3};
        const std::string insert_key{key + "_ms"};
        below_node.put<double>(insert_key, int(coeff * values.at(0)));
        above_node.put<double>(insert_key, int(coeff * values.at(1)));
    }
    {
        const auto values{cfg.get<std::vector<double>>(tower_hop_height_opt)};
        const double coeff{1e6};
        const std::string insert_key{tower_hop_height_opt + "_nm"};
        below_node.put<double>(insert_key, int(coeff * values.at(0)));
        above_node.put<double>(insert_key, int(coeff * values.at(1)));
    }
    for (const std::string& key : speed_opts) {
        const auto values{cfg.get<Domain::EnumVectorWrapper>(key).get_strings()};
        const std::string insert_key{boost::replace_all_copy(key, "_speed", "_profile")};
        below_node.put(insert_key, values.at(0));
        above_node.put(insert_key, values.at(1));
    }
    {
        const auto values{cfg.get<std::vector<bool>>(use_tilt_opt)};
        below_node.put<bool>(use_tilt_opt, values.at(0));
        above_node.put<bool>(use_tilt_opt, values.at(1));
    }
    for (const std::string& key : count_opts) {
        const auto values{cfg.get<std::vector<int>>(key)};
        below_node.put<int>(key, values.at(0));
        above_node.put<int>(key, values.at(1));
    }

    pt::ptree profile_node;
    profile_node.put("area_fill", cfg.get<double>("area_fill"));
    profile_node.add_child("below_area_fill", below_node);
    profile_node.add_child("above_area_fill", above_node);

    pt::ptree root;

    for (auto& param : iniconf) {
        root.put(param.first, param.second );
    }

    root.put("version", "1");
    root.add_child("exposure_profile", profile_node);

    // Boost confirms its implementation has no 100% conformance to JSON standard.
    // In the boost libraries, boost will always serialize each value as string and parse all values to a string equivalent.
    // so, post-prosess output
    return write_json_with_post_process(root);
}

static std::string serialize(const double value)
{
    std::ostringstream ss;
    if (std::isfinite(value))
        ss << value;
    else if (std::isnan(value)) {
        throw std::runtime_error("Serializing NaN");
    } else
        throw std::runtime_error("Serializing invalid number");
    return ss.str();
}

void fill_iniconf(ConfMap &m, const SLAPrintConfigView &cfg, const Sla::PrintStatistics &stats) {
    using Domain::SLAMaterialSpeed;
    using Domain::SLAMaterialSpeed::slamsSlow;
    using Domain::SLAMaterialSpeed::slamsFast;

    CNumericLocalesSetter locales_setter; // for to_string
    m["layerHeight"]    = serialize(cfg.get<double>("layer_height"));
    m["expTime"]        = serialize(cfg.get<double>("exposure_time"));
    m["expTimeFirst"]   = serialize(cfg.get<double>("initial_exposure_time"));
    const Domain::SLAMaterialSpeed mps = cfg.get<Domain::SLAMaterialSpeed>("material_print_speed");
    m["expUserProfile"] = mps == slamsSlow ? "1" : mps == slamsFast ? "0" : "2";

    // TODO commented out, until we know how to reference the settings
    //m["materialName"]   = cfg.get<std::string>("sla_material_settings_id");
    m["printerModel"]   = cfg.get<std::string>("printer_model");
    m["printerVariant"] = cfg.get<std::string>("printer_variant");
    //m["printerProfile"] = cfg.get<std::string>("printer_settings_id");
    //m["printProfile"]   = cfg.get<std::string>("sla_print_settings_id");
    m["fileCreationTimestamp"] = Utils::utc_timestamp();
    m["prusaSlicerVersion"]    = SLIC3R_BUILD_ID;

    // Set statistics values to the printer
    double used_material = (stats.objects_used_material +
                            stats.support_used_material) / 1000;
    m["usedMaterial"] = std::to_string(used_material);
    m["numFade"]      = std::to_string(stats.count_faded_layers);
    m["numSlow"]      = std::to_string(stats.slow_layers_count);
    m["numFast"]      = std::to_string(stats.fast_layers_count);
    m["printTime"]    = std::to_string(stats.estimated_print_time);
    m["hollow"] = stats.hollowing_enable ? "1" : "0";
    m["action"] = "print";
}

static void write_thumbnail(Zipper &zipper, const ThumbnailData &data)
{
    size_t png_size = 0;

    void  *png_data = tdefl_write_image_to_png_file_in_memory_ex(
         (const void *) data.pixels.data(), data.width, data.height, 4,
         &png_size, MZ_DEFAULT_LEVEL, 1);

    if (png_data != nullptr) {
        zipper.add_entry("thumbnail/thumbnail" + std::to_string(data.width) +
                             "x" + std::to_string(data.height) + ".png",
                         static_cast<const std::uint8_t *>(png_data),
                         png_size);

        mz_free(png_data);
    }
}
}

using namespace Slic3r::Biz::Slicing;
void store_sl1(const std::string& file_path, const SLAResult& data)
{
    std::string layer_extension = ".png";
    Zipper::e_compression compression = Zipper::FAST_COMPRESSION;
    if (data.files.type == Sla::FileDataType::sl1_svg){
        layer_extension = ".svg";
        compression = Zipper::TIGHT_COMPRESSION;
    }

    Zipper zipper{file_path, compression};
    std::string project = data.project_name.empty() ?
        boost::filesystem::path(zipper.get_filename()).stem().string() :
        data.project_name;

    const Sla::PrintStatistics& stats = *data.print_statistics;

    const Biz::Print::SerializedConfig& serialized_config{data.serialized_config};
    const SLAPrintConfigView full_config{data.print_config};

    ConfMap iniconf;
    fill_iniconf(iniconf, full_config, stats);

    iniconf["jobDir"] = project;

    try {
        zipper.add_entry("config.ini");
        zipper << to_ini(iniconf);
        zipper.add_entry("config.json");
        zipper << tilt_options_to_json(full_config, iniconf);

        zipper.add_entry("prusaslicer.ini");
        zipper << serialized_config.ini;
        zipper.add_entry("prusaslicer.json");
        zipper << serialized_config.json;


        size_t i = 0;
        for (const Sla::FileData& rst : data.files.data) {
            std::string imgname = project + string_printf("%.5d", i++) + layer_extension;
            zipper.add_entry(imgname.c_str(), rst.data(), rst.size());
        }

        for (const ThumbnailData& data : data.thumbnails)
            if (data.is_valid())
                write_thumbnail(zipper, data);

        zipper.finalize();
    } catch(std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << e.what();
        // Rethrow the exception
        throw;
    }
}

using namespace Slic3r;
using namespace Slic3r::sla;
class Sl1Rasterizer : public ISlaRasterizer
{
    Resolution res;
    PixelDim pxdim;
    double gamma;
    RasterBase::Trafo tr;

public:
    explicit Sl1Rasterizer(const SLAPrintConfigView& cfg) {
        double w = cfg.get<double>("display_width");
        double h = cfg.get<double>("display_height");
        auto pw = size_t(cfg.get<int>("display_pixels_x"));
        auto ph = size_t(cfg.get<int>("display_pixels_y"));

        std::array<bool, 2> mirror;
        mirror[X] = cfg.get<bool>("display_mirror_x");
        mirror[Y] = cfg.get<bool>("display_mirror_y");

        auto ro = cfg.get<Domain::SLADisplayOrientation>("display_orientation");
        RasterBase::Orientation orientation = ro == Domain::SLADisplayOrientation::sladoPortrait
            ? RasterBase::roPortrait
            : RasterBase::roLandscape;

        if (orientation == RasterBase::roPortrait) {
            std::swap(w, h);
            std::swap(pw, ph);
        }

        res = Resolution{pw, ph};
        pxdim = PixelDim{w / pw, h / ph};
        gamma = cfg.get<double>("gamma_correction");
        tr = RasterBase::Trafo{orientation, mirror};
    }

    Sla::FileData create_file(const ExPolygons& slice) override {
        std::unique_ptr<sla::RasterBase> raster = create_raster_grayscale_aa(res, pxdim, gamma, tr);
        for (const ExPolygon& part : slice)
            raster->draw(part);

        sla::RasterEncoder encoder = sla::PNGRasterEncoder{};
        EncodedRaster encoded_raster = raster->encode(encoder);
        return std::move(encoded_raster.m_buffer);
    }
};

std::unique_ptr<Slic3r::ISlaRasterizer> create_sl1_rasterizer(const SLAPrintConfigView& cfg){
    return std::make_unique<Sl1Rasterizer>(cfg);
}
} // namespace Slic3r
