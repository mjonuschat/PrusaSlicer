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
#include "libslic3r/SLAPrint.hpp"
#include "Slic3r/Exception.hpp"
#include "libslic3r/MTUtils.hpp"
#include "libslic3r/PrintConfig.hpp"

#include "libslic3r/miniz_extension.hpp" // IWYU pragma: keep
#include <LocalesUtils.hpp>
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "libslic3r/Utils/JsonUtils.hpp"

#include "SLAArchiveReader.hpp"
#include "SLAArchiveFormatRegistry.hpp"
#include "ZipperArchiveImport.hpp"

#include "libslic3r/MarchingSquares.hpp"
#include "libslic3r/PNGReadWrite.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/Execution/ExecutionTBB.hpp"

#include "libslic3r/SLA/RasterBase.hpp"


#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

using namespace Slic3r::Biz::Slicing;

namespace Slic3r {

namespace execution = Slic3r::Biz::Algorithms::Execution;
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
}

std::unique_ptr<Slic3r::ISlaRasterizer> Slic3r::create_sl1_rasterizer(const SLAPrintConfigView& cfg){
    return std::make_unique<Sl1Rasterizer>(cfg);
}

// /////////////////////////////////////////////////////////////////////////////
// Reader implementation
// /////////////////////////////////////////////////////////////////////////////

namespace marchsq {

template<> struct _RasterTraits<Slic3r::png::ImageGreyscale> {
    using Rst = Slic3r::png::ImageGreyscale;

       // The type of pixel cell in the raster
    using ValueType = uint8_t;

       // Value at a given position
    static uint8_t get(const Rst &rst, size_t row, size_t col)
    {
        return rst.get(row, col);
    }

       // Number of rows and cols of the raster
    static size_t rows(const Rst &rst) { return rst.rows; }
    static size_t cols(const Rst &rst) { return rst.cols; }
};

} // namespace marchsq

namespace Slic3r {

template<class Fn> static void foreach_vertex(ExPolygon &poly, Fn &&fn)
{
    for (auto &p : poly.contour.points) fn(p);
    for (auto &h : poly.holes)
        for (auto &p : h.points) fn(p);
}

void invert_raster_trafo(ExPolygons &                  expolys,
                         const sla::RasterBase::Trafo &trafo,
                         coord_t                       width,
                         coord_t                       height)
{
    if (trafo.flipXY) std::swap(height, width);

    for (auto &expoly : expolys) {
        if (trafo.mirror_y)
            foreach_vertex(expoly, [height](Point &p) {p.y() = height - p.y(); });

        if (trafo.mirror_x)
            foreach_vertex(expoly, [width](Point &p) {p.x() = width - p.x(); });

        expoly.translate(-trafo.center_x, -trafo.center_y);

        if (trafo.flipXY)
            foreach_vertex(expoly, [](Point &p) { std::swap(p.x(), p.y()); });

        if ((trafo.mirror_x + trafo.mirror_y + trafo.flipXY) % 2) {
            expoly.contour.reverse();
            for (auto &h : expoly.holes) h.reverse();
        }
    }
}

RasterParams get_raster_params(const DynamicPrintConfig &cfg)
{
    auto *opt_disp_cols = cfg.option<ConfigOptionInt>("display_pixels_x");
    auto *opt_disp_rows = cfg.option<ConfigOptionInt>("display_pixels_y");
    auto *opt_disp_w    = cfg.option<ConfigOptionFloat>("display_width");
    auto *opt_disp_h    = cfg.option<ConfigOptionFloat>("display_height");
    auto *opt_mirror_x  = cfg.option<ConfigOptionBool>("display_mirror_x");
    auto *opt_mirror_y  = cfg.option<ConfigOptionBool>("display_mirror_y");
    auto *opt_orient    = cfg.option<ConfigOptionEnum<SLADisplayOrientation>>("display_orientation");

    if (!opt_disp_cols || !opt_disp_rows || !opt_disp_w || !opt_disp_h ||
        !opt_mirror_x || !opt_mirror_y || !opt_orient)
        throw MissingProfileError("Invalid SL1 / SL1S file");

    RasterParams rstp;

    rstp.px_w = opt_disp_w->value / (opt_disp_cols->value - 1);
    rstp.px_h = opt_disp_h->value / (opt_disp_rows->value - 1);

    rstp.trafo = sla::RasterBase::Trafo{opt_orient->value == sladoLandscape ?
                                            sla::RasterBase::roLandscape :
                                            sla::RasterBase::roPortrait,
                                        {opt_mirror_x->value, opt_mirror_y->value}};

    rstp.height = scaled(opt_disp_h->value);
    rstp.width  = scaled(opt_disp_w->value);

    return rstp;
}

namespace {

ExPolygons rings_to_expolygons(const std::vector<marchsq::Ring> &rings,
                               double px_w, double px_h)
{
    auto polys = reserve_vector<ExPolygon>(rings.size());

    for (const marchsq::Ring &ring : rings) {
        Polygon poly; Points &pts = poly.points;
        pts.reserve(ring.size());

        for (const marchsq::Coord &crd : ring)
            pts.emplace_back(scaled(crd.c * px_w), scaled(crd.r * px_h));

        polys.emplace_back(poly);
    }

    // TODO: Is a union necessary?
    return union_ex(polys);
}

std::vector<ExPolygons> extract_slices_from_sla_archive(
    ZipperArchive           &arch,
    const RasterParams      &rstp,
    const marchsq::Coord    &win,
    std::function<bool(int)> progr)
{
    std::vector<ExPolygons> slices(arch.entries.size());

    struct Status
    {
        double                                 incr, val, prev;
        bool                                   stop  = false;
        execution::SpinningMutex<execution::ExecutionTBB> mutex = {};
    } st{100. / slices.size(), 0., 0.};

    execution::for_each(
        execution::ex_tbb, size_t(0), arch.entries.size(),
        [&arch, &slices, &st, &rstp, &win, progr](size_t i) {
            // Status indication guarded with the spinlock
            {
                std::lock_guard lck(st.mutex);
                if (st.stop) return;

                st.val += st.incr;
                double curr = std::round(st.val);
                if (curr > st.prev) {
                    st.prev = curr;
                    st.stop = !progr(int(curr));
                }
            }

            png::ImageGreyscale img;
            png::ReadBuf        rb{arch.entries[i].buf.data(),
                            arch.entries[i].buf.size()};
            if (!png::decode_png(rb, img)) return;

            constexpr uint8_t isoval = 128;
            auto              rings = marchsq::execute(img, isoval, win);
            ExPolygons        expolys = rings_to_expolygons(rings, rstp.px_w,
                                                            rstp.px_h);

            // Invert the raster transformations indicated in the profile metadata
            invert_raster_trafo(expolys, rstp.trafo, rstp.width, rstp.height);

            slices[i] = std::move(expolys);
        },
        execution::max_concurrency(execution::ex_tbb));

    if (st.stop) slices = {};

    return slices;
}

} // namespace

ConfigSubstitutions SL1Reader::read(std::vector<ExPolygons> &slices,
                                    DynamicPrintConfig      &profile_out)
{
    std::array<int, 2> windowsize;

    switch(m_quality)
    {
    case SLAImportQuality::Fast: windowsize = {8, 8}; break;
    case SLAImportQuality::Balanced: windowsize = {4, 4}; break;
    default:
    case SLAImportQuality::Accurate:
        windowsize = {2, 2}; break;
    };

    // Ensure minimum window size for marching squares
    windowsize[0] = std::max(2, windowsize[0]);
    windowsize[1] = std::max(2, windowsize[1]);

    std::vector<std::string> includes = { "ini", "png"};
    std::vector<std::string> excludes = { "thumbnail" };
    ZipperArchive arch = read_zipper_archive(m_fname, includes, excludes);
    auto [profile_use, config_substitutions] = extract_profile(arch, profile_out);

    RasterParams   rstp = get_raster_params(profile_use);
    marchsq::Coord win  = {windowsize[1], windowsize[0]};
    slices = extract_slices_from_sla_archive(arch, rstp, win, m_progr);

    return std::move(config_substitutions);
}

ConfigSubstitutions SL1Reader::read(DynamicPrintConfig &out)
{
    ZipperArchive arch = read_zipper_archive(m_fname, {"ini"}, {"png", "thumbnail"});
    return out.load(arch.profile, ForwardCompatibilitySubstitutionRule::Enable);
}

} // namespace Slic3r
