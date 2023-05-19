#include "Config.hpp"
#include "libslic3r.h"
#include "GCode/ExtrusionProcessor.hpp"
#include "I18N.hpp"
#include "GCode.hpp"
#include "Exception.hpp"
#include "ExtrusionEntity.hpp"
#include "Geometry/ConvexHull.hpp"
#include "GCode/PrintExtents.hpp"
#include "GCode/Thumbnails.hpp"
#include "GCode/WipeTower.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "PrintConfig.hpp"
#include "ShortestPath.hpp"
#include "Print.hpp"
#include "Thread.hpp"
#include "Utils.hpp"
#include "ClipperUtils.hpp"
#include "libslic3r.h"
#include "LocalesUtils.hpp"
#include "libslic3r/format.hpp"

#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <math.h>
#include <string>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/cstdlib.hpp>

#include "SVG.hpp"

#include <tbb/parallel_for.h>

// Intel redesigned some TBB interface considerably when merging TBB with their oneAPI set of libraries, see GH #7332.
// We are using quite an old TBB 2017 U7. Before we update our build servers, let's use the old API, which is deprecated in up to date TBB.
#if ! defined(TBB_VERSION_MAJOR)
    #include <tbb/version.h>
#endif
#if ! defined(TBB_VERSION_MAJOR)
    static_assert(false, "TBB_VERSION_MAJOR not defined");
#endif
#if TBB_VERSION_MAJOR >= 2021
    #include <tbb/parallel_pipeline.h>
    using slic3r_tbb_filtermode = tbb::filter_mode;
#else
    #include <tbb/pipeline.h>
    using slic3r_tbb_filtermode = tbb::filter;
#endif

using namespace std::literals::string_view_literals;

#if 0
// Enable debugging and asserts, even in the release build.
#define DEBUG
#define _DEBUG
#undef NDEBUG
#endif

#include <assert.h>

namespace Slic3r {

// Only add a newline in case the current G-code does not end with a newline.
    static inline void check_add_eol(std::string& gcode)
    {
        if (!gcode.empty() && gcode.back() != '\n')
            gcode += '\n';
    }


    // Return true if tch_prefix is found in custom_gcode
    static bool custom_gcode_changes_tool(const std::string& custom_gcode, const std::string& tch_prefix, unsigned next_extruder)
    {
        bool ok = false;
        size_t from_pos = 0;
        size_t pos = 0;
        while ((pos = custom_gcode.find(tch_prefix, from_pos)) != std::string::npos) {
            if (pos + 1 == custom_gcode.size())
                break;
            from_pos = pos + 1;
            // only whitespace is allowed before the command
            while (--pos < custom_gcode.size() && custom_gcode[pos] != '\n') {
                if (!std::isspace(custom_gcode[pos]))
                    goto NEXT;
            }
            {
                // we should also check that the extruder changes to what was expected
                std::istringstream ss(custom_gcode.substr(from_pos, std::string::npos));
                unsigned num = 0;
                if (ss >> num)
                    ok = (num == next_extruder);
            }
        NEXT:;
        }
        return ok;
    }

    inline std::string make_klipper_exclude_object_name(const std::string &name, int object_id, int instance_id)
    {
        constexpr char banned_chars[] = "-. \r\n\v\t\f";
        std::string cleaned_name = name;
        const char *next = cleaned_name.c_str();
        while (next = std::strpbrk(next, banned_chars))
            cleaned_name[next - cleaned_name.c_str()] = '_';
        return cleaned_name + "_id_" + std::to_string(object_id) + "_copy_" + std::to_string(instance_id);
    }

    // Label excluded objects for Klipper
    std::string make_klipper_exclude_object_header(const Print &print) {
        std::string output;
        int object_id = 0;
        for (auto object : print.objects()) {
            int instance_id = 0;
            for (auto instance : object->instances()) {
                char buffer[64];
                output += "EXCLUDE_OBJECT_DEFINE NAME=";
                output += make_klipper_exclude_object_name(object->model_object()->name, object_id, instance_id++);
                Polygon outline = object->model_object()->convex_hull_2d(instance.model_instance->get_matrix());
                outline.douglas_peucker(50000.f);
                auto center = outline.centroid();
                std::snprintf(buffer, sizeof(buffer) - 1, " CENTER=%.3f,%.3f", unscale<float>(center[0]), unscale<float>(center[1]));
                output += buffer + std::string(" POLYGON=[");
                for (auto point : outline) {
                    std::snprintf(buffer, sizeof(buffer) - 1, "[%.3f,%.3f],", unscale<float>(point[0]), unscale<float>(point[1]));
                    output += buffer;
                }
                output.pop_back();
                output += "]\n";
            }
            ++object_id;
        }
        return output;
    }

    std::string OozePrevention::pre_toolchange(GCode& gcodegen)
    {
        std::string gcode;

        unsigned int extruder_id = gcodegen.writer().extruder()->id();
        const ConfigOptionIntsNullable& filament_idle_temp = gcodegen.config().idle_temperature;
        if (filament_idle_temp.is_nil(extruder_id)) {
            // There is no idle temperature defined in filament settings.
            // Use the delta value from print config.
            if (gcodegen.config().standby_temperature_delta.value != 0) {
                // we assume that heating is always slower than cooling, so no need to block
                gcode += gcodegen.writer().set_temperature
                (this->_get_temp(gcodegen) + gcodegen.config().standby_temperature_delta.value, false, extruder_id);
                gcode.pop_back();
                gcode += " ;cooldown\n"; // this is a marker for GCodeProcessor, so it can supress the commands when needed
            }
        } else {
            // Use the value from filament settings. That one is absolute, not delta.
            gcode += gcodegen.writer().set_temperature(filament_idle_temp.get_at(extruder_id), false, extruder_id);
            gcode.pop_back();
            gcode += " ;cooldown\n"; // this is a marker for GCodeProcessor, so it can supress the commands when needed
        }

        return gcode;
    }

    std::string OozePrevention::post_toolchange(GCode& gcodegen)
    {
        return (gcodegen.config().standby_temperature_delta.value != 0) ?
            gcodegen.writer().set_temperature(this->_get_temp(gcodegen), true, gcodegen.writer().extruder()->id()) :
            std::string();
    }

    int OozePrevention::_get_temp(const GCode& gcodegen) const
    {
        return (gcodegen.layer() == nullptr || gcodegen.layer()->id() == 0)
            ? gcodegen.config().first_layer_temperature.get_at(gcodegen.writer().extruder()->id())
            : gcodegen.config().temperature.get_at(gcodegen.writer().extruder()->id());
    }

    std::string Wipe::wipe(GCode& gcodegen, bool toolchange)
    {
        std::string     gcode;
        const Extruder &extruder = *gcodegen.writer().extruder();

        // Remaining quantized retraction length.
        if (double retract_length = extruder.retract_to_go(toolchange ? extruder.retract_length_toolchange() : extruder.retract_length()); 
            retract_length > 0 && this->path.size() >= 2) {
            // Reduce feedrate a bit; travel speed is often too high to move on existing material.
            // Too fast = ripping of existing material; too slow = short wipe path, thus more blob.
            const double wipe_speed = gcodegen.writer().config.travel_speed.value * 0.8;
            // Reduce retraction length a bit to avoid effective retraction speed to be greater than the configured one
            // due to rounding (TODO: test and/or better math for this).
            const double xy_to_e    = 0.95 * extruder.retract_speed() / wipe_speed;
            // Start with the current position, which may be different from the wipe path start in case of loop clipping.
            Vec2d prev = gcodegen.point_to_gcode_quantized(gcodegen.last_pos());
            auto  it   = this->path.points.begin();
            Vec2d p    = gcodegen.point_to_gcode_quantized(*(++ it));
            if (p != prev) {
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Start) + "\n";
                auto  end  = this->path.points.end();
                bool  done = false;
                for (; it != end && ! done; ++ it) {
                    p = gcodegen.point_to_gcode_quantized(*it);
                    double segment_length = (p - prev).norm();
                    double dE = GCodeFormatter::quantize_e(xy_to_e * segment_length);
                    if (dE > retract_length - EPSILON) {
                        if (dE > retract_length + EPSILON)
                            // Shorten the segment.
                            p = prev + (p - prev) * (retract_length / dE);
                        dE   = retract_length;
                        done = true;
                    }
                    //FIXME one shall not generate the unnecessary G1 Fxxx commands, here wipe_speed is a constant inside this cycle.
                    // Is it here for the cooling markers? Or should it be outside of the cycle?
                    gcode += gcodegen.writer().set_speed(wipe_speed * 60, {}, gcodegen.enable_cooling_markers() ? ";_WIPE" : "");
                    gcode += gcodegen.writer().extrude_to_xy(p, -dE, "wipe and retract");
                    prev = p;
                    retract_length -= dE;
                }
                // add tag for processor
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_End) + "\n";
                gcodegen.set_last_pos(gcodegen.gcode_to_point(prev));
            }
        }

        // Prevent wiping again on the same path.
        this->reset_path();
        return gcode;
    }

    static inline Point wipe_tower_point_to_object_point(GCode& gcodegen, const Vec2f& wipe_tower_pt)
    {
        return Point(scale_(wipe_tower_pt.x() - gcodegen.origin()(0)), scale_(wipe_tower_pt.y() - gcodegen.origin()(1)));
    }

    std::string WipeTowerIntegration::append_tcr(GCode& gcodegen, const WipeTower::ToolChangeResult& tcr, int new_extruder_id, double z) const
    {
        if (new_extruder_id != -1 && new_extruder_id != tcr.new_tool)
            throw Slic3r::InvalidArgument("Error: WipeTowerIntegration::append_tcr was asked to do a toolchange it didn't expect.");

        std::string gcode;

        // Toolchangeresult.gcode assumes the wipe tower corner is at the origin (except for priming lines)
        // We want to rotate and shift all extrusions (gcode postprocessing) and starting and ending position
        float alpha = m_wipe_tower_rotation / 180.f * float(M_PI);

        auto transform_wt_pt = [&alpha, this](const Vec2f& pt) -> Vec2f {
            Vec2f out = Eigen::Rotation2Df(alpha) * pt;
            out += m_wipe_tower_pos;
            return out;
        };

        Vec2f start_pos = tcr.start_pos;
        Vec2f end_pos = tcr.end_pos;
        if (! tcr.priming) {
            start_pos = transform_wt_pt(start_pos);
            end_pos = transform_wt_pt(end_pos);
        }

        Vec2f wipe_tower_offset = tcr.priming ? Vec2f::Zero() : m_wipe_tower_pos;
        float wipe_tower_rotation = tcr.priming ? 0.f : alpha;

        std::string tcr_rotated_gcode = post_process_wipe_tower_moves(tcr, wipe_tower_offset, wipe_tower_rotation);

        gcode += gcodegen.writer().unlift(); // Make sure there is no z-hop (in most cases, there isn't).

        double current_z = gcodegen.writer().get_position().z();
        if (z == -1.) // in case no specific z was provided, print at current_z pos
            z = current_z;

        const bool needs_toolchange = gcodegen.writer().need_toolchange(new_extruder_id);
        const bool will_go_down = ! is_approx(z, current_z);
        const bool is_ramming = (gcodegen.config().single_extruder_multi_material && ! tcr.priming)
                             || (! gcodegen.config().single_extruder_multi_material && gcodegen.config().filament_multitool_ramming.get_at(tcr.initial_tool));
        const bool should_travel_to_tower = tcr.force_travel        // wipe tower says so
                                         || ! needs_toolchange      // this is just finishing the tower with no toolchange
                                         || is_ramming;
        if (should_travel_to_tower) {
            // FIXME: It would be better if the wipe tower set the force_travel flag for all toolchanges,
            // then we could simplify the condition and make it more readable.
            gcode += gcodegen.retract();
            gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
            gcode += gcodegen.travel_to(
                wipe_tower_point_to_object_point(gcodegen, start_pos),
                ExtrusionRole::Mixed,
                "Travel to a Wipe Tower");
            gcode += gcodegen.unretract();
        } else {
            // When this is multiextruder printer without any ramming, we can just change
            // the tool without travelling to the tower.
        }
        
        if (will_go_down) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(z, "Travel down to the last wipe tower layer.");
            gcode += gcodegen.writer().unretract();
        }

        std::string toolchange_gcode_str;
        std::string deretraction_str;
        if (tcr.priming || (new_extruder_id >= 0 && needs_toolchange)) {
            if (is_ramming)
                gcodegen.m_wipe.reset_path(); // We don't want wiping on the ramming lines.
            toolchange_gcode_str = gcodegen.set_extruder(new_extruder_id, tcr.print_z); // TODO: toolchange_z vs print_z
            if (gcodegen.config().wipe_tower)
                deretraction_str = gcodegen.unretract();
        }

        


        // Insert the toolchange and deretraction gcode into the generated gcode.
        DynamicConfig config;
        config.set_key_value("toolchange_gcode", new ConfigOptionString(toolchange_gcode_str));
        config.set_key_value("deretraction_from_wipe_tower_generator", new ConfigOptionString(deretraction_str));
        std::string tcr_gcode, tcr_escaped_gcode = gcodegen.placeholder_parser_process("tcr_rotated_gcode", tcr_rotated_gcode, new_extruder_id, &config);
        unescape_string_cstyle(tcr_escaped_gcode, tcr_gcode);
        gcode += tcr_gcode;
        check_add_eol(toolchange_gcode_str);

        // A phony move to the end position at the wipe tower.
        gcodegen.writer().travel_to_xy(end_pos.cast<double>());
        gcodegen.set_last_pos(wipe_tower_point_to_object_point(gcodegen, end_pos));
        if (!is_approx(z, current_z)) {
            gcode += gcodegen.writer().retract();
            gcode += gcodegen.writer().travel_to_z(current_z, "Travel back up to the topmost object layer.");
            gcode += gcodegen.writer().unretract();
        }

        else {
            // Prepare a future wipe.
            gcodegen.m_wipe.reset_path();
            for (const Vec2f& wipe_pt : tcr.wipe_path)
                gcodegen.m_wipe.path.points.emplace_back(wipe_tower_point_to_object_point(gcodegen, transform_wt_pt(wipe_pt)));
        }

        // Let the planner know we are traveling between objects.
        gcodegen.m_avoid_crossing_perimeters.use_external_mp_once();
        return gcode;
    }

    // This function postprocesses gcode_original, rotates and moves all G1 extrusions and returns resulting gcode
    // Starting position has to be supplied explicitely (otherwise it would fail in case first G1 command only contained one coordinate)
    std::string WipeTowerIntegration::post_process_wipe_tower_moves(const WipeTower::ToolChangeResult& tcr, const Vec2f& translation, float angle) const
    {
        Vec2f extruder_offset = m_extruder_offsets[tcr.initial_tool].cast<float>();

        std::istringstream gcode_str(tcr.gcode);
        std::string gcode_out;
        std::string line;
        Vec2f pos = tcr.start_pos;
        Vec2f transformed_pos = Eigen::Rotation2Df(angle) * pos + translation;
        Vec2f old_pos(-1000.1f, -1000.1f);

        while (gcode_str) {
            std::getline(gcode_str, line);  // we read the gcode line by line

            // All G1 commands should be translated and rotated. X and Y coords are
            // only pushed to the output when they differ from last time.
            // WT generator can override this by appending the never_skip_tag
            if (boost::starts_with(line, "G1 ")) {
                bool never_skip = false;
                auto it = line.find(WipeTower::never_skip_tag());
                if (it != std::string::npos) {
                    // remove the tag and remember we saw it
                    never_skip = true;
                    line.erase(it, it + WipeTower::never_skip_tag().size());
                }
                std::ostringstream line_out;
                std::istringstream line_str(line);
                line_str >> std::noskipws;  // don't skip whitespace
                char ch = 0;
                line_str >> ch >> ch; // read the "G1"
                while (line_str >> ch) {
                    if (ch == 'X' || ch == 'Y')
                        line_str >> (ch == 'X' ? pos.x() : pos.y());
                    else
                        line_out << ch;
                }

                transformed_pos = Eigen::Rotation2Df(angle) * pos + translation;

                if (transformed_pos != old_pos || never_skip) {
                    line = line_out.str();
                    boost::trim_left(line); // Remove leading spaces
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(3) << "G1";
                    if (transformed_pos.x() != old_pos.x() || never_skip)
                        oss << " X" << transformed_pos.x() - extruder_offset.x();
                    if (transformed_pos.y() != old_pos.y() || never_skip)
                        oss << " Y" << transformed_pos.y() - extruder_offset.y();
                    if (! line.empty())
                        oss << " ";
                    line = oss.str() + line;
                    old_pos = transformed_pos;
                }
            }

            gcode_out += line + "\n";

            // If this was a toolchange command, we should change current extruder offset
            if (line == "[toolchange_gcode]") {
                extruder_offset = m_extruder_offsets[tcr.new_tool].cast<float>();

                // If the extruder offset changed, add an extra move so everything is continuous
                if (extruder_offset != m_extruder_offsets[tcr.initial_tool].cast<float>()) {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(3)
                        << "G1 X" << transformed_pos.x() - extruder_offset.x()
                        << " Y" << transformed_pos.y() - extruder_offset.y()
                        << "\n";
                    gcode_out += oss.str();
                }
            }
        }
        return gcode_out;
    }


    std::string WipeTowerIntegration::prime(GCode& gcodegen)
    {
        std::string gcode;
        for (const WipeTower::ToolChangeResult& tcr : m_priming) {
            if (! tcr.extrusions.empty())
                gcode += append_tcr(gcodegen, tcr, tcr.new_tool);
        }
        return gcode;
    }

    std::string WipeTowerIntegration::tool_change(GCode& gcodegen, int extruder_id, bool finish_layer)
    {
        std::string gcode;
        assert(m_layer_idx >= 0);
        if (gcodegen.writer().need_toolchange(extruder_id) || finish_layer) {
            if (m_layer_idx < (int)m_tool_changes.size()) {
                if (!(size_t(m_tool_change_idx) < m_tool_changes[m_layer_idx].size()))
                    throw Slic3r::RuntimeError("Wipe tower generation failed, possibly due to empty first layer.");

                // Calculate where the wipe tower layer will be printed. -1 means that print z will not change,
                // resulting in a wipe tower with sparse layers.
                double wipe_tower_z = -1;
                bool ignore_sparse = false;
                if (gcodegen.config().wipe_tower_no_sparse_layers.value) {
                    wipe_tower_z = m_last_wipe_tower_print_z;
                    ignore_sparse = (m_tool_changes[m_layer_idx].size() == 1 && m_tool_changes[m_layer_idx].front().initial_tool == m_tool_changes[m_layer_idx].front().new_tool && m_layer_idx != 0);
                    if (m_tool_change_idx == 0 && !ignore_sparse)
                        wipe_tower_z = m_last_wipe_tower_print_z + m_tool_changes[m_layer_idx].front().layer_height;
                }

                if (!ignore_sparse) {
                    gcode += append_tcr(gcodegen, m_tool_changes[m_layer_idx][m_tool_change_idx++], extruder_id, wipe_tower_z);
                    m_last_wipe_tower_print_z = wipe_tower_z;
                }
            }
        }
        return gcode;
    }

    // Print is finished. Now it remains to unload the filament safely with ramming over the wipe tower.
    std::string WipeTowerIntegration::finalize(GCode& gcodegen)
    {
        std::string gcode;
        if (std::abs(gcodegen.writer().get_position().z() - m_final_purge.print_z) > EPSILON)
            gcode += gcodegen.change_layer(m_final_purge.print_z);
        gcode += append_tcr(gcodegen, m_final_purge, -1);
        return gcode;
    }

    const std::vector<std::string> ColorPrintColors::Colors = { "#C0392B", "#E67E22", "#F1C40F", "#27AE60", "#1ABC9C", "#2980B9", "#9B59B6" };

#define EXTRUDER_CONFIG(OPT) m_config.OPT.get_at(m_writer.extruder()->id())

void GCode::PlaceholderParserIntegration::reset()
{
    this->failed_templates.clear();
    this->output_config.clear();
    this->opt_position = nullptr;
    this->opt_zhop      = nullptr;
    this->opt_e_position = nullptr;
    this->opt_e_retracted = nullptr;
    this->opt_e_restart_extra = nullptr;
    this->opt_extruded_volume = nullptr;
    this->opt_extruded_weight = nullptr;
    this->opt_extruded_volume_total = nullptr;
    this->opt_extruded_weight_total = nullptr;
    this->num_extruders = 0;
    this->position.clear();
    this->e_position.clear();
    this->e_retracted.clear();
    this->e_restart_extra.clear();
}

void GCode::PlaceholderParserIntegration::init(const GCodeWriter &writer)
{
    this->reset();
    const std::vector<Extruder> &extruders = writer.extruders();
    if (! extruders.empty()) {
        this->num_extruders = extruders.back().id() + 1;
        this->e_retracted.assign(num_extruders, 0);
        this->e_restart_extra.assign(num_extruders, 0);
        this->opt_e_retracted = new ConfigOptionFloats(e_retracted);
        this->opt_e_restart_extra = new ConfigOptionFloats(e_restart_extra);
        this->output_config.set_key_value("e_retracted", this->opt_e_retracted);
        this->output_config.set_key_value("e_restart_extra", this->opt_e_restart_extra);
        if (! writer.config.use_relative_e_distances) {
            e_position.assign(num_extruders, 0);
            opt_e_position = new ConfigOptionFloats(e_position);
            this->output_config.set_key_value("e_position", opt_e_position);
        }
    }
    this->opt_extruded_volume = new ConfigOptionFloats(this->num_extruders, 0.f);
    this->opt_extruded_weight = new ConfigOptionFloats(this->num_extruders, 0.f);
    this->opt_extruded_volume_total = new ConfigOptionFloat(0.f);
    this->opt_extruded_weight_total = new ConfigOptionFloat(0.f);
    this->parser.set("extruded_volume", this->opt_extruded_volume);
    this->parser.set("extruded_weight", this->opt_extruded_weight);
    this->parser.set("extruded_volume_total", this->opt_extruded_volume_total);
    this->parser.set("extruded_weight_total", this->opt_extruded_weight_total);

    // Reserve buffer for current position.
    this->position.assign(3, 0);
    this->opt_position = new ConfigOptionFloats(this->position);
    this->output_config.set_key_value("position", this->opt_position);
    // Store zhop variable into the parser itself, it is a read-only variable to the script.
    this->opt_zhop = new ConfigOptionFloat(writer.get_zhop());
    this->parser.set("zhop", this->opt_zhop);
}

void GCode::PlaceholderParserIntegration::update_from_gcodewriter(const GCodeWriter &writer)
{
    memcpy(this->position.data(), writer.get_position().data(), sizeof(double) * 3);
    this->opt_position->values = this->position;
    this->opt_zhop->value = writer.get_zhop();

    if (this->num_extruders > 0) {
        const std::vector<Extruder> &extruders = writer.extruders();
        assert(! extruders.empty() && num_extruders == extruders.back().id() + 1);
        this->e_retracted.assign(num_extruders, 0);
        this->e_restart_extra.assign(num_extruders, 0);
        this->opt_extruded_volume->values.assign(num_extruders, 0);
        this->opt_extruded_weight->values.assign(num_extruders, 0);
        double total_volume = 0.;
        double total_weight = 0.;
        for (const Extruder &e : extruders) {
            this->e_retracted[e.id()]     = e.retracted();
            this->e_restart_extra[e.id()] = e.restart_extra();
            double v = e.extruded_volume();
            double w = v * e.filament_density() * 0.001;
            this->opt_extruded_volume->values[e.id()] = v;
            this->opt_extruded_weight->values[e.id()] = w;
            total_volume += v;
            total_weight += w;
        }
        opt_extruded_volume_total->value = total_volume;
        opt_extruded_weight_total->value = total_weight;
        opt_e_retracted->values = this->e_retracted;
        opt_e_restart_extra->values = this->e_restart_extra;
        if (! writer.config.use_relative_e_distances) {
            this->e_position.assign(num_extruders, 0);
            for (const Extruder &e : extruders)
                this->e_position[e.id()] = e.position();
            this->opt_e_position->values = this->e_position;
        }
    }
}

// Throw if any of the output vector variables were resized by the script.
void GCode::PlaceholderParserIntegration::validate_output_vector_variables()
{
    if (this->opt_position->values.size() != 3)
        throw Slic3r::RuntimeError("\"position\" output variable must not be resized by the script.");
    if (this->num_extruders > 0) {
        if (this->opt_e_position && this->opt_e_position->values.size() != this->num_extruders)
            throw Slic3r::RuntimeError("\"e_position\" output variable must not be resized by the script.");
        if (this->opt_e_retracted->values.size() != this->num_extruders)
            throw Slic3r::RuntimeError("\"e_retracted\" output variable must not be resized by the script.");
        if (this->opt_e_restart_extra->values.size() != this->num_extruders)
            throw Slic3r::RuntimeError("\"e_restart_extra\" output variable must not be resized by the script.");
    }
}

// Collect pairs of object_layer + support_layer sorted by print_z.
// object_layer & support_layer are considered to be on the same print_z, if they are not further than EPSILON.
GCode::ObjectsLayerToPrint GCode::collect_layers_to_print(const PrintObject& object)
{
    GCode::ObjectsLayerToPrint layers_to_print;
    layers_to_print.reserve(object.layers().size() + object.support_layers().size());

    /*
    // Calculate a minimum support layer height as a minimum over all extruders, but not smaller than 10um.
    // This is the same logic as in support generator.
    //FIXME should we use the printing extruders instead?
    double gap_over_supports = object.config().support_material_contact_distance;
    // FIXME should we test object.config().support_material_synchronize_layers ? Currently the support layers are synchronized with object layers iff soluble supports.
    assert(!object.has_support() || gap_over_supports != 0. || object.config().support_material_synchronize_layers);
    if (gap_over_supports != 0.) {
        gap_over_supports = std::max(0., gap_over_supports);
        // Not a soluble support,
        double support_layer_height_min = 1000000.;
        for (auto lh : object.print()->config().min_layer_height.values)
            support_layer_height_min = std::min(support_layer_height_min, std::max(0.01, lh));
        gap_over_supports += support_layer_height_min;
    }*/

    std::vector<std::pair<double, double>> warning_ranges;

    // Pair the object layers with the support layers by z.
    size_t idx_object_layer = 0;
    size_t idx_support_layer = 0;
    const ObjectLayerToPrint* last_extrusion_layer = nullptr;
    while (idx_object_layer < object.layers().size() || idx_support_layer < object.support_layers().size()) {
        ObjectLayerToPrint layer_to_print;
        layer_to_print.object_layer = (idx_object_layer < object.layers().size()) ? object.layers()[idx_object_layer++] : nullptr;
        layer_to_print.support_layer = (idx_support_layer < object.support_layers().size()) ? object.support_layers()[idx_support_layer++] : nullptr;
        if (layer_to_print.object_layer && layer_to_print.support_layer) {
            if (layer_to_print.object_layer->print_z < layer_to_print.support_layer->print_z - EPSILON) {
                layer_to_print.support_layer = nullptr;
                --idx_support_layer;
            }
            else if (layer_to_print.support_layer->print_z < layer_to_print.object_layer->print_z - EPSILON) {
                layer_to_print.object_layer = nullptr;
                --idx_object_layer;
            }
        }

        layers_to_print.emplace_back(layer_to_print);

        bool has_extrusions = (layer_to_print.object_layer && layer_to_print.object_layer->has_extrusions())
            || (layer_to_print.support_layer && layer_to_print.support_layer->has_extrusions());

        // Check that there are extrusions on the very first layer. The case with empty
        // first layer may result in skirt/brim in the air and maybe other issues.
        if (layers_to_print.size() == 1u) {
            if (!has_extrusions)
                throw Slic3r::SlicingError(_u8L("There is an object with no extrusions in the first layer.") + "\n" +
                                           _u8L("Object name") + ": " + object.model_object()->name);
        }

        // In case there are extrusions on this layer, check there is a layer to lay it on.
        if ((layer_to_print.object_layer && layer_to_print.object_layer->has_extrusions())
            // Allow empty support layers, as the support generator may produce no extrusions for non-empty support regions.
            || (layer_to_print.support_layer /* && layer_to_print.support_layer->has_extrusions() */)) {
            double top_cd = object.config().support_material_contact_distance;
            double bottom_cd = object.config().support_material_bottom_contact_distance == 0. ? top_cd : object.config().support_material_bottom_contact_distance;

            double extra_gap = (layer_to_print.support_layer ? bottom_cd : top_cd);

            double maximal_print_z = (last_extrusion_layer ? last_extrusion_layer->print_z() : 0.)
                + layer_to_print.layer()->height
                + std::max(0., extra_gap);
            // Negative support_contact_z is not taken into account, it can result in false positives in cases
            // where previous layer has object extrusions too (https://github.com/prusa3d/PrusaSlicer/issues/2752)

            if (has_extrusions && layer_to_print.print_z() > maximal_print_z + 2. * EPSILON)
                warning_ranges.emplace_back(std::make_pair((last_extrusion_layer ? last_extrusion_layer->print_z() : 0.), layers_to_print.back().print_z()));
        }
        // Remember last layer with extrusions.
        if (has_extrusions)
            last_extrusion_layer = &layers_to_print.back();
    }

    if (! warning_ranges.empty()) {
        std::string warning;
        size_t i = 0;
        for (i = 0; i < std::min(warning_ranges.size(), size_t(3)); ++i)
            warning += Slic3r::format(_u8L("Empty layer between %1% and %2%."),
                                      warning_ranges[i].first, warning_ranges[i].second) + "\n";
        if (i < warning_ranges.size())
            warning += _u8L("(Some lines not shown)") + "\n";
        warning += "\n";
        warning += Slic3r::format(_u8L("Object name: %1%"), object.model_object()->name) + "\n\n"
            + _u8L("Make sure the object is printable. This is usually caused by negligibly small extrusions or by a faulty model. "
                "Try to repair the model or change its orientation on the bed.");

        const_cast<Print*>(object.print())->active_step_add_warning(
            PrintStateBase::WarningLevel::CRITICAL, warning);
    }

    return layers_to_print;
}

// Prepare for non-sequential printing of multiple objects: Support resp. object layers with nearly identical print_z
// will be printed for  all objects at once.
// Return a list of <print_z, per object ObjectLayerToPrint> items.
std::vector<std::pair<coordf_t, GCode::ObjectsLayerToPrint>> GCode::collect_layers_to_print(const Print& print)
{
    struct OrderingItem {
        coordf_t    print_z;
        size_t      object_idx;
        size_t      layer_idx;
    };

    std::vector<ObjectsLayerToPrint>  per_object(print.objects().size(), ObjectsLayerToPrint());
    std::vector<OrderingItem>         ordering;
    for (size_t i = 0; i < print.objects().size(); ++i) {
        per_object[i] = collect_layers_to_print(*print.objects()[i]);
        OrderingItem ordering_item;
        ordering_item.object_idx = i;
        ordering.reserve(ordering.size() + per_object[i].size());
        const ObjectLayerToPrint &front = per_object[i].front();
        for (const ObjectLayerToPrint &ltp : per_object[i]) {
            ordering_item.print_z = ltp.print_z();
            ordering_item.layer_idx = &ltp - &front;
            ordering.emplace_back(ordering_item);
        }
    }

    std::sort(ordering.begin(), ordering.end(), [](const OrderingItem& oi1, const OrderingItem& oi2) { return oi1.print_z < oi2.print_z; });

    std::vector<std::pair<coordf_t, ObjectsLayerToPrint>> layers_to_print;

    // Merge numerically very close Z values.
    for (size_t i = 0; i < ordering.size();) {
        // Find the last layer with roughly the same print_z.
        size_t j = i + 1;
        coordf_t zmax = ordering[i].print_z + EPSILON;
        for (; j < ordering.size() && ordering[j].print_z <= zmax; ++j);
        // Merge into layers_to_print.
        std::pair<coordf_t, ObjectsLayerToPrint> merged;
        // Assign an average print_z to the set of layers with nearly equal print_z.
        merged.first = 0.5 * (ordering[i].print_z + ordering[j - 1].print_z);
        merged.second.assign(print.objects().size(), ObjectLayerToPrint());
        for (; i < j; ++i) {
            const OrderingItem& oi = ordering[i];
            assert(merged.second[oi.object_idx].layer() == nullptr);
            merged.second[oi.object_idx] = std::move(per_object[oi.object_idx][oi.layer_idx]);
        }
        layers_to_print.emplace_back(std::move(merged));
    }

    return layers_to_print;
}

// free functions called by GCode::do_export()
namespace DoExport {
//    static void update_print_estimated_times_stats(const GCodeProcessor& processor, PrintStatistics& print_statistics)
//    {
//        const GCodeProcessorResult& result = processor.get_result();
//        print_statistics.estimated_normal_print_time = get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time);
//        print_statistics.estimated_silent_print_time = processor.is_stealth_time_estimator_enabled() ?
//            get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].time) : "N/A";
//    }

    static void update_print_estimated_stats(const GCodeProcessor& processor, const std::vector<Extruder>& extruders, PrintStatistics& print_statistics)
    {
        const GCodeProcessorResult& result = processor.get_result();
        print_statistics.estimated_normal_print_time = get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time);
        print_statistics.estimated_silent_print_time = processor.is_stealth_time_estimator_enabled() ?
            get_time_dhms(result.print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].time) : "N/A";

        // update filament statictics
        double total_extruded_volume = 0.0;
        double total_used_filament   = 0.0;
        double total_weight          = 0.0;
        double total_cost            = 0.0;
        for (auto volume : result.print_statistics.volumes_per_extruder) {
            total_extruded_volume += volume.second;

            size_t extruder_id = volume.first;
            auto extruder = std::find_if(extruders.begin(), extruders.end(), [extruder_id](const Extruder& extr) { return extr.id() == extruder_id; });
            if (extruder == extruders.end())
                continue;

            double s = PI * sqr(0.5* extruder->filament_diameter());
            double weight = volume.second * extruder->filament_density() * 0.001;
            total_used_filament += volume.second/s;
            total_weight        += weight;
            total_cost          += weight * extruder->filament_cost() * 0.001;
        }

        print_statistics.total_extruded_volume = total_extruded_volume;
        print_statistics.total_used_filament   = total_used_filament;
        print_statistics.total_weight          = total_weight;
        print_statistics.total_cost            = total_cost;

        print_statistics.filament_stats        = result.print_statistics.volumes_per_extruder;
    }

    // if any reserved keyword is found, returns a std::vector containing the first MAX_COUNT keywords found
    // into pairs containing:
    // first: source
    // second: keyword
    // to be shown in the warning notification
    // The returned vector is empty if no keyword has been found
    static std::vector<std::pair<std::string, std::string>> validate_custom_gcode(const Print& print) {
        static const unsigned int MAX_TAGS_COUNT = 5;
        std::vector<std::pair<std::string, std::string>> ret;

        auto check = [&ret](const std::string& source, const std::string& gcode) {
            std::vector<std::string> tags;
            if (GCodeProcessor::contains_reserved_tags(gcode, MAX_TAGS_COUNT, tags)) {
                if (!tags.empty()) {
                    size_t i = 0;
                    while (ret.size() < MAX_TAGS_COUNT && i < tags.size()) {
                        ret.push_back({ source, tags[i] });
                        ++i;
                    }
                }
            }
        };

        const GCodeConfig& config = print.config();
        check(_u8L("Start G-code"), config.start_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("End G-code"), config.end_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Before layer change G-code"), config.before_layer_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("After layer change G-code"), config.layer_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Tool change G-code"), config.toolchange_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Between objects G-code (for sequential printing)"), config.between_objects_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Color Change G-code"), config.color_change_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Pause Print G-code"), config.pause_print_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) check(_u8L("Template Custom G-code"), config.template_custom_gcode.value);
        if (ret.size() < MAX_TAGS_COUNT) {
            for (const std::string& value : config.start_filament_gcode.values) {
                check(_u8L("Filament Start G-code"), value);
                if (ret.size() == MAX_TAGS_COUNT)
                    break;
            }
        }
        if (ret.size() < MAX_TAGS_COUNT) {
            for (const std::string& value : config.end_filament_gcode.values) {
                check(_u8L("Filament End G-code"), value);
                if (ret.size() == MAX_TAGS_COUNT)
                    break;
            }
        }
        if (ret.size() < MAX_TAGS_COUNT) {
            const CustomGCode::Info& custom_gcode_per_print_z = print.model().custom_gcode_per_print_z;
            for (const auto& gcode : custom_gcode_per_print_z.gcodes) {
                check(_u8L("Custom G-code"), gcode.extra);
                if (ret.size() == MAX_TAGS_COUNT)
                    break;
            }
        }

        return ret;
    }
} // namespace DoExport

void GCode::do_export(Print* print, const char* path, GCodeProcessorResult* result, ThumbnailsGeneratorCallback thumbnail_cb)
{
    CNumericLocalesSetter locales_setter;

    // Does the file exist? If so, we hope that it is still valid.
    {
        PrintStateBase::StateWithTimeStamp state = print->step_state_with_timestamp(psGCodeExport);
        if (! state.enabled || (state.is_done() && boost::filesystem::exists(boost::filesystem::path(path))))
            return;
    }

    // Enabled and either not done, or marked as done while the output file is missing.
    print->set_started(psGCodeExport);

    // check if any custom gcode contains keywords used by the gcode processor to
    // produce time estimation and gcode toolpaths
    std::vector<std::pair<std::string, std::string>> validation_res = DoExport::validate_custom_gcode(*print);
    if (!validation_res.empty()) {
        std::string reports;
        for (const auto& [source, keyword] : validation_res) {
            reports += source + ": \"" + keyword + "\"\n";
        }
        print->active_step_add_warning(PrintStateBase::WarningLevel::NON_CRITICAL,
            _u8L("In the custom G-code were found reserved keywords:") + "\n" +
            reports +
            _u8L("This may cause problems in g-code visualization and printing time estimation."));
    }

    BOOST_LOG_TRIVIAL(info) << "Exporting G-code..." << log_memory_info();

    // Remove the old g-code if it exists.
    boost::nowide::remove(path);

    std::string path_tmp(path);
    path_tmp += ".tmp";

    m_processor.initialize(path_tmp);
    m_processor.set_print(print);
    GCodeOutputStream file(boost::nowide::fopen(path_tmp.c_str(), "wb"), m_processor);
    if (! file.is_open())
        throw Slic3r::RuntimeError(std::string("G-code export to ") + path + " failed.\nCannot open the file for writing.\n");

    try {
        this->_do_export(*print, file, thumbnail_cb);
        file.flush();
        if (file.is_error()) {
            file.close();
            boost::nowide::remove(path_tmp.c_str());
            throw Slic3r::RuntimeError(std::string("G-code export to ") + path + " failed\nIs the disk full?\n");
        }
    } catch (std::exception & /* ex */) {
        // Rethrow on any exception. std::runtime_exception and CanceledException are expected to be thrown.
        // Close and remove the file.
        file.close();
        boost::nowide::remove(path_tmp.c_str());
        throw;
    }
    file.close();

    if (! m_placeholder_parser_integration.failed_templates.empty()) {
        // G-code export proceeded, but some of the PlaceholderParser substitutions failed.
        //FIXME localize!
        std::string msg = std::string("G-code export to ") + path + " failed due to invalid custom G-code sections:\n\n";
        for (const auto &name_and_error : m_placeholder_parser_integration.failed_templates)
            msg += name_and_error.first + "\n" + name_and_error.second + "\n";
        msg += "\nPlease inspect the file ";
        msg += path_tmp + " for error messages enclosed between\n";
        msg += "        !!!!! Failed to process the custom G-code template ...\n";
        msg += "and\n";
        msg += "        !!!!! End of an error report for the custom G-code template ...\n";
        msg += "for all macro processing errors.";
        throw Slic3r::PlaceholderParserError(msg);
    }

    BOOST_LOG_TRIVIAL(debug) << "Start processing gcode, " << log_memory_info();
    // Post-process the G-code to update time stamps.
    m_processor.finalize(true);
//    DoExport::update_print_estimated_times_stats(m_processor, print->m_print_statistics);
    DoExport::update_print_estimated_stats(m_processor, m_writer.extruders(), print->m_print_statistics);
    if (result != nullptr) {
        *result = std::move(m_processor.extract_result());
        // set the filename to the correct value
        result->filename = path;
    }
    BOOST_LOG_TRIVIAL(debug) << "Finished processing gcode, " << log_memory_info();

    if (rename_file(path_tmp, path))
        throw Slic3r::RuntimeError(
            std::string("Failed to rename the output G-code file from ") + path_tmp + " to " + path + '\n' +
            "Is " + path_tmp + " locked?" + '\n');

    BOOST_LOG_TRIVIAL(info) << "Exporting G-code finished" << log_memory_info();
    print->set_done(psGCodeExport);
}

// free functions called by GCode::_do_export()
namespace DoExport {
    static void init_gcode_processor(const PrintConfig& config, GCodeProcessor& processor, bool& silent_time_estimator_enabled)
    {
        silent_time_estimator_enabled = (config.gcode_flavor == gcfMarlinLegacy || config.gcode_flavor == gcfMarlinFirmware)
                                        && config.silent_mode;
        processor.reset();
        processor.initialize_result_moves();
        processor.apply_config(config);
        processor.enable_stealth_time_estimator(silent_time_estimator_enabled);
    }

	static double autospeed_volumetric_limit(const Print &print)
	{
	    // get the minimum cross-section used in the print
	    std::vector<double> mm3_per_mm;
	    for (auto object : print.objects()) {
	        for (size_t region_id = 0; region_id < object->num_printing_regions(); ++ region_id) {
	            const PrintRegion &region = object->printing_region(region_id);
	            for (auto layer : object->layers()) {
	                const LayerRegion* layerm = layer->regions()[region_id];
	                if (region.config().get_abs_value("perimeter_speed") == 0 ||
	                    region.config().get_abs_value("small_perimeter_speed") == 0 ||
	                    region.config().get_abs_value("external_perimeter_speed") == 0 ||
	                    region.config().get_abs_value("bridge_speed") == 0)
	                    mm3_per_mm.push_back(layerm->perimeters().min_mm3_per_mm());
	                if (region.config().get_abs_value("infill_speed") == 0 ||
	                    region.config().get_abs_value("solid_infill_speed") == 0 ||
	                    region.config().get_abs_value("top_solid_infill_speed") == 0 ||
                        region.config().get_abs_value("bridge_speed") == 0)
                    {
                        // Minimal volumetric flow should not be calculated over ironing extrusions.
                        // Use following lambda instead of the built-it method.
                        // https://github.com/prusa3d/PrusaSlicer/issues/5082
                        auto min_mm3_per_mm_no_ironing = [](const ExtrusionEntityCollection& eec) -> double {
                            double min = std::numeric_limits<double>::max();
                            for (const ExtrusionEntity* ee : eec.entities)
                                if (ee->role() != ExtrusionRole::Ironing)
                                    min = std::min(min, ee->min_mm3_per_mm());
                            return min;
                        };

                        mm3_per_mm.push_back(min_mm3_per_mm_no_ironing(layerm->fills()));
                    }
	            }
	        }
	        if (object->config().get_abs_value("support_material_speed") == 0 ||
	            object->config().get_abs_value("support_material_interface_speed") == 0)
	            for (auto layer : object->support_layers())
	                mm3_per_mm.push_back(layer->support_fills.min_mm3_per_mm());
	    }
	    // filter out 0-width segments
	    mm3_per_mm.erase(std::remove_if(mm3_per_mm.begin(), mm3_per_mm.end(), [](double v) { return v < 0.000001; }), mm3_per_mm.end());
	    double volumetric_speed = 0.;
	    if (! mm3_per_mm.empty()) {
	        // In order to honor max_print_speed we need to find a target volumetric
	        // speed that we can use throughout the print. So we define this target 
	        // volumetric speed as the volumetric speed produced by printing the 
	        // smallest cross-section at the maximum speed: any larger cross-section
	        // will need slower feedrates.
	        volumetric_speed = *std::min_element(mm3_per_mm.begin(), mm3_per_mm.end()) * print.config().max_print_speed.value;
	        // limit such volumetric speed with max_volumetric_speed if set
	        if (print.config().max_volumetric_speed.value > 0)
	            volumetric_speed = std::min(volumetric_speed, print.config().max_volumetric_speed.value);
	    }
	    return volumetric_speed;
	}


    static void init_ooze_prevention(const Print &print, OozePrevention &ooze_prevention)
	{
	    ooze_prevention.enable = print.config().ooze_prevention.value && ! print.config().single_extruder_multi_material;
	}

	// Fill in print_statistics and return formatted string containing filament statistics to be inserted into G-code comment section.
    static std::string update_print_stats_and_format_filament_stats(
        const bool                   has_wipe_tower,
	    const WipeTowerData         &wipe_tower_data,
        const FullPrintConfig       &config,
	    const std::vector<Extruder> &extruders,
        unsigned int                 initial_extruder_id,
		PrintStatistics 		    &print_statistics)
    {
		std::string filament_stats_string_out;

	    print_statistics.clear();
        print_statistics.total_toolchanges = std::max(0, wipe_tower_data.number_of_toolchanges);
        print_statistics.initial_extruder_id = initial_extruder_id;
        std::vector<std::string> filament_types;
	    if (! extruders.empty()) {
	        std::pair<std::string, unsigned int> out_filament_used_mm ("; filament used [mm] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_used_cm3("; filament used [cm3] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_used_g  ("; filament used [g] = ", 0);
	        std::pair<std::string, unsigned int> out_filament_cost    ("; filament cost = ", 0);
	        for (const Extruder &extruder : extruders) {
                print_statistics.printing_extruders.emplace_back(extruder.id());
                filament_types.emplace_back(config.filament_type.get_at(extruder.id()));

	            double used_filament   = extruder.used_filament() + (has_wipe_tower ? wipe_tower_data.used_filament[extruder.id()] : 0.f);
	            double extruded_volume = extruder.extruded_volume() + (has_wipe_tower ? wipe_tower_data.used_filament[extruder.id()] * 2.4052f : 0.f); // assumes 1.75mm filament diameter
	            double filament_weight = extruded_volume * extruder.filament_density() * 0.001;
	            double filament_cost   = filament_weight * extruder.filament_cost()    * 0.001;
                auto append = [&extruder](std::pair<std::string, unsigned int> &dst, const char *tmpl, double value) {
                    assert(is_decimal_separator_point());
	                while (dst.second < extruder.id()) {
	                    // Fill in the non-printing extruders with zeros.
	                    dst.first += (dst.second > 0) ? ", 0" : "0";
	                    ++ dst.second;
	                }
	                if (dst.second > 0)
	                    dst.first += ", ";
	                char buf[64];
					sprintf(buf, tmpl, value);
	                dst.first += buf;
	                ++ dst.second;
	            };
	            append(out_filament_used_mm,  "%.2lf", used_filament);
	            append(out_filament_used_cm3, "%.2lf", extruded_volume * 0.001);
	            if (filament_weight > 0.) {
	                print_statistics.total_weight = print_statistics.total_weight + filament_weight;
	                append(out_filament_used_g, "%.2lf", filament_weight);
	                if (filament_cost > 0.) {
	                    print_statistics.total_cost = print_statistics.total_cost + filament_cost;
	                    append(out_filament_cost, "%.2lf", filament_cost);
	                }
	            }
	            print_statistics.total_used_filament += used_filament;
	            print_statistics.total_extruded_volume += extruded_volume;
	            print_statistics.total_wipe_tower_filament += has_wipe_tower ? used_filament - extruder.used_filament() : 0.;
	            print_statistics.total_wipe_tower_cost += has_wipe_tower ? (extruded_volume - extruder.extruded_volume())* extruder.filament_density() * 0.001 * extruder.filament_cost() * 0.001 : 0.;
	        }
	        filament_stats_string_out += out_filament_used_mm.first;
            filament_stats_string_out += "\n" + out_filament_used_cm3.first;
            if (out_filament_used_g.second)
                filament_stats_string_out += "\n" + out_filament_used_g.first;
            if (out_filament_cost.second)
                filament_stats_string_out += "\n" + out_filament_cost.first;
            print_statistics.initial_filament_type = config.filament_type.get_at(initial_extruder_id);
            std::sort(filament_types.begin(), filament_types.end());
            print_statistics.printing_filament_types = filament_types.front();
            for (size_t i = 1; i < filament_types.size(); ++ i) {
                print_statistics.printing_filament_types += ",";
                print_statistics.printing_filament_types += filament_types[i];
            }
        }
        return filament_stats_string_out;
    }
}

#if 0
// Sort the PrintObjects by their increasing Z, likely useful for avoiding colisions on Deltas during sequential prints.
static inline std::vector<const PrintInstance*> sort_object_instances_by_max_z(const Print &print)
{
    std::vector<const PrintObject*> objects(print.objects().begin(), print.objects().end());
    std::sort(objects.begin(), objects.end(), [](const PrintObject *po1, const PrintObject *po2) { return po1->height() < po2->height(); });
    std::vector<const PrintInstance*> instances;
    instances.reserve(objects.size());
    for (const PrintObject *object : objects)
        for (size_t i = 0; i < object->instances().size(); ++ i)
            instances.emplace_back(&object->instances()[i]);
    return instances;
}
#endif

// Produce a vector of PrintObjects in the order of their respective ModelObjects in print.model().
std::vector<const PrintInstance*> sort_object_instances_by_model_order(const Print& print)
{
    // Build up map from ModelInstance* to PrintInstance*
    std::vector<std::pair<const ModelInstance*, const PrintInstance*>> model_instance_to_print_instance;
    model_instance_to_print_instance.reserve(print.num_object_instances());
    for (const PrintObject *print_object : print.objects())
        for (const PrintInstance &print_instance : print_object->instances())
            model_instance_to_print_instance.emplace_back(print_instance.model_instance, &print_instance);
    std::sort(model_instance_to_print_instance.begin(), model_instance_to_print_instance.end(), [](auto &l, auto &r) { return l.first < r.first; });

    std::vector<const PrintInstance*> instances;
    instances.reserve(model_instance_to_print_instance.size());
    for (const ModelObject *model_object : print.model().objects)
        for (const ModelInstance *model_instance : model_object->instances) {
            auto it = std::lower_bound(model_instance_to_print_instance.begin(), model_instance_to_print_instance.end(), std::make_pair(model_instance, nullptr), [](auto &l, auto &r) { return l.first < r.first; });
            if (it != model_instance_to_print_instance.end() && it->first == model_instance)
                instances.emplace_back(it->second);
        }
    return instances;
}

void GCode::_do_export(Print& print, GCodeOutputStream &file, ThumbnailsGeneratorCallback thumbnail_cb)
{
    // modifies m_silent_time_estimator_enabled
    DoExport::init_gcode_processor(print.config(), m_processor, m_silent_time_estimator_enabled);

    if (! print.config().gcode_substitutions.values.empty()) {
        m_find_replace = make_unique<GCodeFindReplace>(print.config());
        file.set_find_replace(m_find_replace.get(), false);
    }

    // resets analyzer's tracking data
    m_last_height  = 0.f;
    m_last_layer_z = 0.f;
    m_max_layer_z  = 0.f;
    m_last_width = 0.f;
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    m_last_mm3_per_mm = 0.;
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    // How many times will be change_layer() called?
    // change_layer() in turn increments the progress bar status.
    m_layer_count = 0;
    if (print.config().complete_objects.value) {
        // Add each of the object's layers separately.
        for (auto object : print.objects()) {
            std::vector<coordf_t> zs;
            zs.reserve(object->layers().size() + object->support_layers().size());
            for (auto layer : object->layers())
                zs.push_back(layer->print_z);
            for (auto layer : object->support_layers())
                zs.push_back(layer->print_z);
            std::sort(zs.begin(), zs.end());
            m_layer_count += (unsigned int)(object->instances().size() * (std::unique(zs.begin(), zs.end()) - zs.begin()));
        }
    }
    print.throw_if_canceled();

    m_enable_cooling_markers = true;
    this->apply_print_config(print.config());

    m_volumetric_speed = DoExport::autospeed_volumetric_limit(print);
    print.throw_if_canceled();

    if (print.config().spiral_vase.value)
        m_spiral_vase = make_unique<SpiralVase>(print.config());

    if (print.config().max_volumetric_extrusion_rate_slope_positive.value > 0 ||
        print.config().max_volumetric_extrusion_rate_slope_negative.value > 0)
        m_pressure_equalizer = make_unique<PressureEqualizer>(print.config());
    m_enable_extrusion_role_markers = (bool)m_pressure_equalizer;

    if (print.config().avoid_crossing_curled_overhangs){
        this->m_avoid_crossing_curled_overhangs.init_bed_shape(get_bed_shape(print.config()));
    }

    // Write information on the generator.
    file.write_format("; %s\n\n", Slic3r::header_slic3r_generated().c_str());

    // Unit tests or command line slicing may not define "thumbnails" or "thumbnails_format".
    // If "thumbnails_format" is not defined, export to PNG.
    if (const auto [thumbnails, thumbnails_format] = std::make_pair(
            print.full_print_config().option<ConfigOptionPoints>("thumbnails"),
            print.full_print_config().option<ConfigOptionEnum<GCodeThumbnailsFormat>>("thumbnails_format"));
        thumbnails)
        GCodeThumbnails::export_thumbnails_to_file(
            thumbnail_cb, thumbnails->values, thumbnails_format ? thumbnails_format->value : GCodeThumbnailsFormat::PNG,
            [&file](const char* sz) { file.write(sz); },
            [&print]() { print.throw_if_canceled(); });

    // Write notes (content of the Print Settings tab -> Notes)
    {
        std::list<std::string> lines;
        boost::split(lines, print.config().notes.value, boost::is_any_of("\n"), boost::token_compress_off);
        for (auto line : lines) {
            // Remove the trailing '\r' from the '\r\n' sequence.
            if (! line.empty() && line.back() == '\r')
                line.pop_back();
            file.write_format("; %s\n", line.c_str());
        }
        if (! lines.empty())
            file.write("\n");
    }
    print.throw_if_canceled();

    // Write some terse information on the slicing parameters.
    const PrintObject *first_object         = print.objects().front();
    const double       layer_height         = first_object->config().layer_height.value;
    assert(! print.config().first_layer_height.percent);
    const double       first_layer_height   = print.config().first_layer_height.value;
    for (size_t region_id = 0; region_id < print.num_print_regions(); ++ region_id) {
        const PrintRegion &region = print.get_print_region(region_id);
        file.write_format("; external perimeters extrusion width = %.2fmm\n", region.flow(*first_object, frExternalPerimeter, layer_height).width());
        file.write_format("; perimeters extrusion width = %.2fmm\n",          region.flow(*first_object, frPerimeter,         layer_height).width());
        file.write_format("; infill extrusion width = %.2fmm\n",              region.flow(*first_object, frInfill,            layer_height).width());
        file.write_format("; solid infill extrusion width = %.2fmm\n",        region.flow(*first_object, frSolidInfill,       layer_height).width());
        file.write_format("; top infill extrusion width = %.2fmm\n",          region.flow(*first_object, frTopSolidInfill,    layer_height).width());
        if (print.has_support_material())
            file.write_format("; support material extrusion width = %.2fmm\n", support_material_flow(first_object).width());
        if (print.config().first_layer_extrusion_width.value > 0)
            file.write_format("; first layer extrusion width = %.2fmm\n",   region.flow(*first_object, frPerimeter, first_layer_height, true).width());
        file.write_format("\n");
    }
    print.throw_if_canceled();

    // adds tags for time estimators
    if (print.config().remaining_times.value)
        file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::First_Line_M73_Placeholder).c_str());

    // Label all instances for Klipper excluded objects
    if (config().gcode_label_objects && config().gcode_flavor == gcfKlipper)
        file.write(make_klipper_exclude_object_header(print));

    // Starting now, the G-code find / replace post-processor will be enabled.
    file.find_replace_enable();

    // Prepare the helper object for replacing placeholders in custom G-code and output filename.
    m_placeholder_parser_integration.parser = print.placeholder_parser();
    m_placeholder_parser_integration.parser.update_timestamp();
    m_placeholder_parser_integration.context.rng = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    // Enable passing global variables between PlaceholderParser invocations.
    m_placeholder_parser_integration.context.global_config = std::make_unique<DynamicConfig>();
    print.update_object_placeholders(m_placeholder_parser_integration.parser.config_writable(), ".gcode");

    // Get optimal tool ordering to minimize tool switches of a multi-exruder print.
    // For a print by objects, find the 1st printing object.
    ToolOrdering tool_ordering;
    unsigned int initial_extruder_id = (unsigned int)-1;
    unsigned int final_extruder_id   = (unsigned int)-1;
    bool         has_wipe_tower      = false;
    std::vector<const PrintInstance*> 					print_object_instances_ordering;
    std::vector<const PrintInstance*>::const_iterator 	print_object_instance_sequential_active;
    if (print.config().complete_objects.value) {
        // Order object instances for sequential print.
        print_object_instances_ordering = sort_object_instances_by_model_order(print);
//        print_object_instances_ordering = sort_object_instances_by_max_z(print);
        // Find the 1st printing object, find its tool ordering and the initial extruder ID.
        print_object_instance_sequential_active = print_object_instances_ordering.begin();
        for (; print_object_instance_sequential_active != print_object_instances_ordering.end(); ++ print_object_instance_sequential_active) {
            tool_ordering = ToolOrdering(*(*print_object_instance_sequential_active)->print_object, initial_extruder_id);
            if ((initial_extruder_id = tool_ordering.first_extruder()) != static_cast<unsigned int>(-1))
                break;
        }
        if (initial_extruder_id == static_cast<unsigned int>(-1))
            // No object to print was found, cancel the G-code export.
            throw Slic3r::SlicingError(_u8L("No extrusions were generated for objects."));
        // We don't allow switching of extruders per layer by Model::custom_gcode_per_print_z in sequential mode.
        // Use the extruder IDs collected from Regions.
        this->set_extruders(print.extruders());
    } else {
        // Find tool ordering for all the objects at once, and the initial extruder ID.
        // If the tool ordering has been pre-calculated by Print class for wipe tower already, reuse it.
        tool_ordering = print.tool_ordering();
        tool_ordering.assign_custom_gcodes(print);
        if (tool_ordering.all_extruders().empty())
            // No object to print was found, cancel the G-code export.
            throw Slic3r::SlicingError(_u8L("No extrusions were generated for objects."));
        has_wipe_tower = print.has_wipe_tower() && tool_ordering.has_wipe_tower();
        initial_extruder_id = (has_wipe_tower && ! print.config().single_extruder_multi_material_priming) ?
            // The priming towers will be skipped.
            tool_ordering.all_extruders().back() :
            // Don't skip the priming towers.
            tool_ordering.first_extruder();
        // In non-sequential print, the printing extruders may have been modified by the extruder switches stored in Model::custom_gcode_per_print_z.
        // Therefore initialize the printing extruders from there.
        this->set_extruders(tool_ordering.all_extruders());
        // Order object instances using a nearest neighbor search.
        print_object_instances_ordering = chain_print_object_instances(print);
        m_layer_count = tool_ordering.layer_tools().size();
    }
    if (initial_extruder_id == (unsigned int)-1) {
        // Nothing to print!
        initial_extruder_id = 0;
        final_extruder_id   = 0;
    } else {
        final_extruder_id = tool_ordering.last_extruder();
        assert(final_extruder_id != (unsigned int)-1);
    }
    print.throw_if_canceled();

    m_cooling_buffer = make_unique<CoolingBuffer>(*this);
    m_cooling_buffer->set_current_extruder(initial_extruder_id);

    // Emit machine envelope limits for the Marlin firmware.
    this->print_machine_envelope(file, print);

    // Update output variables after the extruders were initialized.
    m_placeholder_parser_integration.init(m_writer);
    // Let the start-up script prime the 1st printing tool.
    this->placeholder_parser().set("initial_tool", initial_extruder_id);
    this->placeholder_parser().set("initial_extruder", initial_extruder_id);
    this->placeholder_parser().set("current_extruder", initial_extruder_id);
    //Set variable for total layer count so it can be used in custom gcode.
    this->placeholder_parser().set("total_layer_count", m_layer_count);
    // Useful for sequential prints.
    this->placeholder_parser().set("current_object_idx", 0);
    // For the start / end G-code to do the priming and final filament pull in case there is no wipe tower provided.
    this->placeholder_parser().set("has_wipe_tower", has_wipe_tower);
    this->placeholder_parser().set("has_single_extruder_multi_material_priming", has_wipe_tower && print.config().single_extruder_multi_material_priming);
    this->placeholder_parser().set("total_toolchanges", std::max(0, print.wipe_tower_data().number_of_toolchanges)); // Check for negative toolchanges (single extruder mode) and set to 0 (no tool change).
    {
        BoundingBoxf bbox(print.config().bed_shape.values);
        this->placeholder_parser().set("print_bed_min",  new ConfigOptionFloats({ bbox.min.x(), bbox.min.y() }));
        this->placeholder_parser().set("print_bed_max",  new ConfigOptionFloats({ bbox.max.x(), bbox.max.y() }));
        this->placeholder_parser().set("print_bed_size", new ConfigOptionFloats({ bbox.size().x(), bbox.size().y() }));
    }
    {
        // Convex hull of the 1st layer extrusions, for bed leveling and placing the initial purge line.
        // It encompasses the object extrusions, support extrusions, skirt, brim, wipe tower.
        // It does NOT encompass user extrusions generated by custom G-code,
        // therefore it does NOT encompass the initial purge line.
        // It does NOT encompass MMU/MMU2 starting (wipe) areas.
        auto pts = std::make_unique<ConfigOptionPoints>();
        pts->values.reserve(print.first_layer_convex_hull().size());
        for (const Point &pt : print.first_layer_convex_hull().points)
            pts->values.emplace_back(unscale(pt));
        BoundingBoxf bbox(pts->values);
        this->placeholder_parser().set("first_layer_print_convex_hull", pts.release());
        this->placeholder_parser().set("first_layer_print_min",  new ConfigOptionFloats({ bbox.min.x(), bbox.min.y() }));
        this->placeholder_parser().set("first_layer_print_max",  new ConfigOptionFloats({ bbox.max.x(), bbox.max.y() }));
        this->placeholder_parser().set("first_layer_print_size", new ConfigOptionFloats({ bbox.size().x(), bbox.size().y() }));
        this->placeholder_parser().set("num_extruders", int(print.config().nozzle_diameter.values.size()));
        // PlaceholderParser currently substitues non-existent vector values with the zero'th value, which is harmful in the case of "is_extruder_used[]"
        // as Slicer may lie about availability of such non-existent extruder.
        // We rather sacrifice 256B of memory before we change the behavior of the PlaceholderParser, which should really only fill in the non-existent
        // vector elements for filament parameters.
        std::vector<unsigned char> is_extruder_used(std::max(size_t(255), print.config().nozzle_diameter.size()), 0);
        for (unsigned int extruder_id : tool_ordering.all_extruders())
            is_extruder_used[extruder_id] = true;
        this->placeholder_parser().set("is_extruder_used", new ConfigOptionBools(is_extruder_used));
    }

    // Enable ooze prevention if configured so.
    DoExport::init_ooze_prevention(print, m_ooze_prevention);

    std::string start_gcode = this->placeholder_parser_process("start_gcode", print.config().start_gcode.value, initial_extruder_id);
    // Set bed temperature if the start G-code does not contain any bed temp control G-codes.
    this->_print_first_layer_bed_temperature(file, print, start_gcode, initial_extruder_id, true);
    // Set extruder(s) temperature before and after start G-code.
    this->_print_first_layer_extruder_temperatures(file, print, start_gcode, initial_extruder_id, false);

    // adds tag for processor
    file.write_format(";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), gcode_extrusion_role_to_string(GCodeExtrusionRole::Custom).c_str());

    // Write the custom start G-code
    file.writeln(start_gcode);

    this->_print_first_layer_extruder_temperatures(file, print, start_gcode, initial_extruder_id, true);
    print.throw_if_canceled();

    // Set other general things.
    file.write(this->preamble());

    print.throw_if_canceled();

    // Collect custom seam data from all objects.
    std::function<void(void)> throw_if_canceled_func = [&print]() { print.throw_if_canceled();};
    m_seam_placer.init(print, throw_if_canceled_func);

    if (! (has_wipe_tower && print.config().single_extruder_multi_material_priming)) {
        // Set initial extruder only after custom start G-code.
        // Ugly hack: Do not set the initial extruder if the extruder is primed using the MMU priming towers at the edge of the print bed.
        file.write(this->set_extruder(initial_extruder_id, 0.));
    }

    // Do all objects for each layer.
    if (print.config().complete_objects.value) {
        size_t finished_objects = 0;
        const PrintObject *prev_object = (*print_object_instance_sequential_active)->print_object;
        for (; print_object_instance_sequential_active != print_object_instances_ordering.end(); ++ print_object_instance_sequential_active) {
            const PrintObject &object = *(*print_object_instance_sequential_active)->print_object;
            if (&object != prev_object || tool_ordering.first_extruder() != final_extruder_id) {
                tool_ordering = ToolOrdering(object, final_extruder_id);
                unsigned int new_extruder_id = tool_ordering.first_extruder();
                if (new_extruder_id == (unsigned int)-1)
                    // Skip this object.
                    continue;
                initial_extruder_id = new_extruder_id;
                final_extruder_id   = tool_ordering.last_extruder();
                assert(final_extruder_id != (unsigned int)-1);
            }
            print.throw_if_canceled();
            this->set_origin(unscale((*print_object_instance_sequential_active)->shift));
            if (finished_objects > 0) {
                // Move to the origin position for the copy we're going to print.
                // This happens before Z goes down to layer 0 again, so that no collision happens hopefully.
                m_enable_cooling_markers = false; // we're not filtering these moves through CoolingBuffer
                m_avoid_crossing_perimeters.use_external_mp_once();
                file.write(this->retract());
                file.write(this->travel_to(Point(0, 0), ExtrusionRole::None, "move to origin position for next object"));
                m_enable_cooling_markers = true;
                // Disable motion planner when traveling to first object point.
                m_avoid_crossing_perimeters.disable_once();
                // Ff we are printing the bottom layer of an object, and we have already finished
                // another one, set first layer temperatures. This happens before the Z move
                // is triggered, so machine has more time to reach such temperatures.
                this->placeholder_parser().set("current_object_idx", int(finished_objects));
                std::string between_objects_gcode = this->placeholder_parser_process("between_objects_gcode", print.config().between_objects_gcode.value, initial_extruder_id);
                // Set first layer bed and extruder temperatures, don't wait for it to reach the temperature.
                this->_print_first_layer_bed_temperature(file, print, between_objects_gcode, initial_extruder_id, false);
                this->_print_first_layer_extruder_temperatures(file, print, between_objects_gcode, initial_extruder_id, false);
                file.writeln(between_objects_gcode);
            }
            // Reset the cooling buffer internal state (the current position, feed rate, accelerations).
            m_cooling_buffer->reset(this->writer().get_position());
            m_cooling_buffer->set_current_extruder(initial_extruder_id);
            // Process all layers of a single object instance (sequential mode) with a parallel pipeline:
            // Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
            // and export G-code into file.
            this->process_layers(print, tool_ordering, collect_layers_to_print(object), *print_object_instance_sequential_active - object.instances().data(), file);
            ++ finished_objects;
            // Flag indicating whether the nozzle temperature changes from 1st to 2nd layer were performed.
            // Reset it when starting another object from 1st layer.
            m_second_layer_things_done = false;
            prev_object = &object;
        }
    } else {
        // Sort layers by Z.
        // All extrusion moves with the same top layer height are extruded uninterrupted.
        std::vector<std::pair<coordf_t, ObjectsLayerToPrint>> layers_to_print = collect_layers_to_print(print);
        // Prusa Multi-Material wipe tower.
        if (has_wipe_tower && ! layers_to_print.empty()) {
            m_wipe_tower.reset(new WipeTowerIntegration(print.config(), *print.wipe_tower_data().priming.get(), print.wipe_tower_data().tool_changes, *print.wipe_tower_data().final_purge.get()));
            file.write(m_writer.travel_to_z(first_layer_height + m_config.z_offset.value, "Move to the first layer height"));
            if (print.config().single_extruder_multi_material_priming) {
                file.write(m_wipe_tower->prime(*this));
                // Verify, whether the print overaps the priming extrusions.
                BoundingBoxf bbox_print(get_print_extrusions_extents(print));
                coordf_t twolayers_printz = ((layers_to_print.size() == 1) ? layers_to_print.front() : layers_to_print[1]).first + EPSILON;
                for (const PrintObject *print_object : print.objects())
                    bbox_print.merge(get_print_object_extrusions_extents(*print_object, twolayers_printz));
                bbox_print.merge(get_wipe_tower_extrusions_extents(print, twolayers_printz));
                BoundingBoxf bbox_prime(get_wipe_tower_priming_extrusions_extents(print));
                bbox_prime.offset(0.5f);
                bool overlap = bbox_prime.overlap(bbox_print);

                if (print.config().gcode_flavor == gcfMarlinLegacy || print.config().gcode_flavor == gcfMarlinFirmware) {
                    file.write(this->retract());
                    file.write("M300 S800 P500\n"); // Beep for 500ms, tone 800Hz.
                    if (overlap) {
                        // Wait for the user to remove the priming extrusions.
                        file.write("M1 Remove priming towers and click button.\n");
                    } else {
                        // Just wait for a bit to let the user check, that the priming succeeded.
                        //TODO Add a message explaining what the printer is waiting for. This needs a firmware fix.
                        file.write("M1 S10\n");
                    }
                } else {
                    // This is not Marlin, M1 command is probably not supported.
                    // (See https://github.com/prusa3d/PrusaSlicer/issues/5441.)
                    if (overlap) {
                        print.active_step_add_warning(PrintStateBase::WarningLevel::CRITICAL,
                            _u8L("Your print is very close to the priming regions. "
                              "Make sure there is no collision."));
                    } else {
                        // Just continue printing, no action necessary.
                    }

                }
            }
            print.throw_if_canceled();
        }
        // Process all layers of all objects (non-sequential mode) with a parallel pipeline:
        // Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
        // and export G-code into file.
        this->process_layers(print, tool_ordering, print_object_instances_ordering, layers_to_print, file);
        if (m_wipe_tower)
            // Purge the extruder, pull out the active filament.
            file.write(m_wipe_tower->finalize(*this));
    }

    // Write end commands to file.
    file.write(this->retract());
    file.write(m_writer.set_fan(0));

    // adds tag for processor
    file.write_format(";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), gcode_extrusion_role_to_string(GCodeExtrusionRole::Custom).c_str());

    // Process filament-specific gcode in extruder order.
    {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z",   new ConfigOptionFloat(m_writer.get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        if (print.config().single_extruder_multi_material) {
            // Process the end_filament_gcode for the active filament only.
            int extruder_id = m_writer.extruder()->id();
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(extruder_id));
            file.writeln(this->placeholder_parser_process("end_filament_gcode", print.config().end_filament_gcode.get_at(extruder_id), extruder_id, &config));
        } else {
            for (const std::string &end_gcode : print.config().end_filament_gcode.values) {
                int extruder_id = (unsigned int)(&end_gcode - &print.config().end_filament_gcode.values.front());
                config.set_key_value("filament_extruder_id", new ConfigOptionInt(extruder_id));
                file.writeln(this->placeholder_parser_process("end_filament_gcode", end_gcode, extruder_id, &config));
            }
        }
        file.writeln(this->placeholder_parser_process("end_gcode", print.config().end_gcode, m_writer.extruder()->id(), &config));
    }
    file.write(m_writer.update_progress(m_layer_count, m_layer_count, true)); // 100%
    file.write(m_writer.postamble());

    // From now to the end of G-code, the G-code find / replace post-processor will be disabled.
    // Thus the PrusaSlicer generated config will NOT be processed by the G-code post-processor, see GH issue #7952.
    file.find_replace_supress();

    // adds tags for time estimators
    if (print.config().remaining_times.value)
        file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Last_Line_M73_Placeholder).c_str());

    print.throw_if_canceled();

    // Get filament stats.
    file.write(DoExport::update_print_stats_and_format_filament_stats(
    	// Const inputs
        has_wipe_tower, print.wipe_tower_data(),
        this->config(),
        m_writer.extruders(),
        initial_extruder_id,
        // Modifies
        print.m_print_statistics));
    file.write("\n");
    file.write_format("; total filament used [g] = %.2lf\n", print.m_print_statistics.total_weight);
    file.write_format("; total filament cost = %.2lf\n", print.m_print_statistics.total_cost);
    if (print.m_print_statistics.total_toolchanges > 0)
    	file.write_format("; total toolchanges = %i\n", print.m_print_statistics.total_toolchanges);
    file.write_format(";%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Estimated_Printing_Time_Placeholder).c_str());

    // Append full config, delimited by two 'phony' configuration keys prusaslicer_config = begin and prusaslicer_config = end.
    // The delimiters are structured as configuration key / value pairs to be parsable by older versions of PrusaSlicer G-code viewer.
    {
        file.write("\n; prusaslicer_config = begin\n");
        std::string full_config;
        append_full_config(print, full_config);
        if (!full_config.empty())
            file.write(full_config);
        file.write("; prusaslicer_config = end\n");
    }
    print.throw_if_canceled();
}

// Process all layers of all objects (non-sequential mode) with a parallel pipeline:
// Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
// and export G-code into file.
void GCode::process_layers(
    const Print                                                         &print,
    const ToolOrdering                                                  &tool_ordering,
    const std::vector<const PrintInstance*>                             &print_object_instances_ordering,
    const std::vector<std::pair<coordf_t, ObjectsLayerToPrint>>         &layers_to_print,
    GCodeOutputStream                                                   &output_stream)
{
    // The pipeline is variable: The vase mode filter is optional.
    size_t layer_to_print_idx = 0;
    const auto generator = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [this, &print, &tool_ordering, &print_object_instances_ordering, &layers_to_print, &layer_to_print_idx](tbb::flow_control& fc) -> LayerResult {
            if (layer_to_print_idx >= layers_to_print.size()) {
                if ((!m_pressure_equalizer && layer_to_print_idx == layers_to_print.size()) || (m_pressure_equalizer && layer_to_print_idx == (layers_to_print.size() + 1))) {
                    fc.stop();
                    return {};
                } else {
                    // Pressure equalizer need insert empty input. Because it returns one layer back.
                    // Insert NOP (no operation) layer;
                    ++layer_to_print_idx;
                    return LayerResult::make_nop_layer_result();
                }
            } else {
                const std::pair<coordf_t, ObjectsLayerToPrint> &layer = layers_to_print[layer_to_print_idx++];
                const LayerTools& layer_tools = tool_ordering.tools_for_layer(layer.first);
                if (m_wipe_tower && layer_tools.has_wipe_tower)
                    m_wipe_tower->next_layer();
                print.throw_if_canceled();
                return this->process_layer(print, layer.second, layer_tools, &layer == &layers_to_print.back(), &print_object_instances_ordering, size_t(-1));
            }
        });
    const auto spiral_vase = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [spiral_vase = this->m_spiral_vase.get()](LayerResult in) -> LayerResult {
            if (in.nop_layer_result)
                return in;

            spiral_vase->enable(in.spiral_vase_enable);
            return { spiral_vase->process_layer(std::move(in.gcode)), in.layer_id, in.spiral_vase_enable, in.cooling_buffer_flush};
        });
    const auto pressure_equalizer = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [pressure_equalizer = this->m_pressure_equalizer.get()](LayerResult in) -> LayerResult {
            return pressure_equalizer->process_layer(std::move(in));
        });
    const auto cooling = tbb::make_filter<LayerResult, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [cooling_buffer = this->m_cooling_buffer.get()](LayerResult in) -> std::string {
             if (in.nop_layer_result)
                return in.gcode;

             return cooling_buffer->process_layer(std::move(in.gcode), in.layer_id, in.cooling_buffer_flush);
        });
    const auto find_replace = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [find_replace = this->m_find_replace.get()](std::string s) -> std::string {
            return find_replace->process_layer(std::move(s));
        });
    const auto output = tbb::make_filter<std::string, void>(slic3r_tbb_filtermode::serial_in_order,
        [&output_stream](std::string s) { output_stream.write(s); }
    );

    // It registers a handler that sets locales to "C" before any TBB thread starts participating in tbb::parallel_pipeline.
    // Handler is unregistered when the destructor is called.
    TBBLocalesSetter locales_setter;

    // The pipeline elements are joined using const references, thus no copying is performed.
    output_stream.find_replace_supress();
    if (m_spiral_vase && m_find_replace && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_vase & pressure_equalizer & cooling & find_replace & output);
    else if (m_spiral_vase && m_find_replace)
        tbb::parallel_pipeline(12, generator & spiral_vase &                      cooling & find_replace & output);
    else if (m_spiral_vase && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_vase & pressure_equalizer & cooling &                output);
    else if (m_find_replace && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator &               pressure_equalizer & cooling & find_replace & output);
    else if (m_spiral_vase)
        tbb::parallel_pipeline(12, generator & spiral_vase &                      cooling &                output);
    else if (m_find_replace)
        tbb::parallel_pipeline(12, generator &                                    cooling & find_replace & output);
    else if (m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator &               pressure_equalizer & cooling &                output);
    else
        tbb::parallel_pipeline(12, generator &                                    cooling &                output);
    output_stream.find_replace_enable();
}

// Process all layers of a single object instance (sequential mode) with a parallel pipeline:
// Generate G-code, run the filters (vase mode, cooling buffer), run the G-code analyser
// and export G-code into file.
void GCode::process_layers(
    const Print                             &print,
    const ToolOrdering                      &tool_ordering,
    ObjectsLayerToPrint                      layers_to_print,
    const size_t                             single_object_idx,
    GCodeOutputStream                       &output_stream)
{
    // The pipeline is variable: The vase mode filter is optional.
    size_t layer_to_print_idx = 0;
    const auto generator = tbb::make_filter<void, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [this, &print, &tool_ordering, &layers_to_print, &layer_to_print_idx, single_object_idx](tbb::flow_control& fc) -> LayerResult {
            if (layer_to_print_idx >= layers_to_print.size()) {
                if ((!m_pressure_equalizer && layer_to_print_idx == layers_to_print.size()) || (m_pressure_equalizer && layer_to_print_idx == (layers_to_print.size() + 1))) {
                    fc.stop();
                    return {};
                } else {
                    // Pressure equalizer need insert empty input. Because it returns one layer back.
                    // Insert NOP (no operation) layer;
                    ++layer_to_print_idx;
                    return LayerResult::make_nop_layer_result();
                }
            } else {
                ObjectLayerToPrint &layer = layers_to_print[layer_to_print_idx ++];
                print.throw_if_canceled();
                return this->process_layer(print, { std::move(layer) }, tool_ordering.tools_for_layer(layer.print_z()), &layer == &layers_to_print.back(), nullptr, single_object_idx);
            }
        });
    const auto spiral_vase = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [spiral_vase = this->m_spiral_vase.get()](LayerResult in)->LayerResult {
            if (in.nop_layer_result)
                return in;
            spiral_vase->enable(in.spiral_vase_enable);
            return { spiral_vase->process_layer(std::move(in.gcode)), in.layer_id, in.spiral_vase_enable, in.cooling_buffer_flush };
        });
    const auto pressure_equalizer = tbb::make_filter<LayerResult, LayerResult>(slic3r_tbb_filtermode::serial_in_order,
        [pressure_equalizer = this->m_pressure_equalizer.get()](LayerResult in) -> LayerResult {
             return pressure_equalizer->process_layer(std::move(in));
        });
    const auto cooling = tbb::make_filter<LayerResult, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [cooling_buffer = this->m_cooling_buffer.get()](LayerResult in)->std::string {
            if (in.nop_layer_result)
                return in.gcode;
            return cooling_buffer->process_layer(std::move(in.gcode), in.layer_id, in.cooling_buffer_flush);
        });
    const auto find_replace = tbb::make_filter<std::string, std::string>(slic3r_tbb_filtermode::serial_in_order,
        [find_replace = this->m_find_replace.get()](std::string s) -> std::string {
            return find_replace->process_layer(std::move(s));
        });
    const auto output = tbb::make_filter<std::string, void>(slic3r_tbb_filtermode::serial_in_order,
        [&output_stream](std::string s) { output_stream.write(s); }
    );

    // It registers a handler that sets locales to "C" before any TBB thread starts participating in tbb::parallel_pipeline.
    // Handler is unregistered when the destructor is called.
    TBBLocalesSetter locales_setter;

    // The pipeline elements are joined using const references, thus no copying is performed.
    output_stream.find_replace_supress();
    if (m_spiral_vase && m_find_replace && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_vase & pressure_equalizer & cooling & find_replace & output);
    else if (m_spiral_vase && m_find_replace)
        tbb::parallel_pipeline(12, generator & spiral_vase &                      cooling & find_replace & output);
    else if (m_spiral_vase && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator & spiral_vase & pressure_equalizer & cooling &                output);
    else if (m_find_replace && m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator &               pressure_equalizer & cooling & find_replace & output);
    else if (m_spiral_vase)
        tbb::parallel_pipeline(12, generator & spiral_vase &                      cooling &                output);
    else if (m_find_replace)
        tbb::parallel_pipeline(12, generator &                                    cooling & find_replace & output);
    else if (m_pressure_equalizer)
        tbb::parallel_pipeline(12, generator &               pressure_equalizer & cooling &                output);
    else
        tbb::parallel_pipeline(12, generator &                                    cooling &                output);
    output_stream.find_replace_enable();
}

std::string GCode::placeholder_parser_process(
    const std::string   &name,
    const std::string   &templ,
    unsigned int         current_extruder_id,
    const DynamicConfig *config_override)
{
    PlaceholderParserIntegration &ppi = m_placeholder_parser_integration;
    try {
        ppi.update_from_gcodewriter(m_writer);
        std::string output = ppi.parser.process(templ, current_extruder_id, config_override, &ppi.output_config, &ppi.context);
        ppi.validate_output_vector_variables();

        if (const std::vector<double> &pos = ppi.opt_position->values; ppi.position != pos) {
            // Update G-code writer.
            m_writer.update_position({ pos[0], pos[1], pos[2] });
            this->set_last_pos(this->gcode_to_point({ pos[0], pos[1] }));
        }

        for (const Extruder &e : m_writer.extruders()) {
            unsigned int eid = e.id();
            assert(eid < ppi.num_extruders);
            if ( eid < ppi.num_extruders) {
                if (! m_writer.config.use_relative_e_distances && ! is_approx(ppi.e_position[eid], ppi.opt_e_position->values[eid]))
                    const_cast<Extruder&>(e).set_position(ppi.opt_e_position->values[eid]);
                if (! is_approx(ppi.e_retracted[eid], ppi.opt_e_retracted->values[eid]) || 
                    ! is_approx(ppi.e_restart_extra[eid], ppi.opt_e_restart_extra->values[eid]))
                    const_cast<Extruder&>(e).set_retracted(ppi.opt_e_retracted->values[eid], ppi.opt_e_restart_extra->values[eid]);
            }
        }

        return output;
    } 
    catch (std::runtime_error &err) 
    {
        // Collect the names of failed template substitutions for error reporting.
        auto it = ppi.failed_templates.find(name);
        if (it == ppi.failed_templates.end())
            // Only if there was no error reported for this template, store the first error message into the map to be reported.
            // We don't want to collect error message for each and every occurence of a single custom G-code section.
            ppi.failed_templates.insert(it, std::make_pair(name, std::string(err.what())));
        // Insert the macro error message into the G-code.
        return
            std::string("\n!!!!! Failed to process the custom G-code template ") + name + "\n" +
            err.what() +
            "!!!!! End of an error report for the custom G-code template " + name + "\n\n";
    }
}

// Parse the custom G-code, try to find mcode_set_temp_dont_wait and mcode_set_temp_and_wait or optionally G10 with temperature inside the custom G-code.
// Returns true if one of the temp commands are found, and try to parse the target temperature value into temp_out.
static bool custom_gcode_sets_temperature(const std::string &gcode, const int mcode_set_temp_dont_wait, const int mcode_set_temp_and_wait, const bool include_g10, int &temp_out)
{
    temp_out = -1;
    if (gcode.empty())
        return false;

    const char *ptr = gcode.data();
    bool temp_set_by_gcode = false;
    while (*ptr != 0) {
        // Skip whitespaces.
        for (; *ptr == ' ' || *ptr == '\t'; ++ ptr);
        if (*ptr == 'M' || // Line starts with 'M'. It is a machine command.
            (*ptr == 'G' && include_g10)) { // Only check for G10 if requested
            bool is_gcode = *ptr == 'G';
            ++ ptr;
            // Parse the M or G code value.
            char *endptr = nullptr;
            int mgcode = int(strtol(ptr, &endptr, 10));
            if (endptr != nullptr && endptr != ptr && 
                is_gcode ?
                    // G10 found
                    mgcode == 10 :
                    // M104/M109 or M140/M190 found.
                    (mgcode == mcode_set_temp_dont_wait || mgcode == mcode_set_temp_and_wait)) {
                ptr = endptr;
                if (! is_gcode)
                    // Let the caller know that the custom M-code sets the temperature.
                    temp_set_by_gcode = true;
                // Now try to parse the temperature value.
                // While not at the end of the line:
                while (strchr(";\r\n\0", *ptr) == nullptr) {
                    // Skip whitespaces.
                    for (; *ptr == ' ' || *ptr == '\t'; ++ ptr);
                    if (*ptr == 'S') {
                        // Skip whitespaces.
                        for (++ ptr; *ptr == ' ' || *ptr == '\t'; ++ ptr);
                        // Parse an int.
                        endptr = nullptr;
                        long temp_parsed = strtol(ptr, &endptr, 10);
                        if (endptr > ptr) {
                            ptr = endptr;
                            temp_out = temp_parsed;
                            // Let the caller know that the custom G-code sets the temperature
                            // Only do this after successfully parsing temperature since G10
                            // can be used for other reasons
                            temp_set_by_gcode = true;
                        }
                    } else {
                        // Skip this word.
                        for (; strchr(" \t;\r\n\0", *ptr) == nullptr; ++ ptr);
                    }
                }
            }
        }
        // Skip the rest of the line.
        for (; *ptr != 0 && *ptr != '\r' && *ptr != '\n'; ++ ptr);
        // Skip the end of line indicators.
        for (; *ptr == '\r' || *ptr == '\n'; ++ ptr);
    }
    return temp_set_by_gcode;
}

// Print the machine envelope G-code for the Marlin firmware based on the "machine_max_xxx" parameters.
// Do not process this piece of G-code by the time estimator, it already knows the values through another sources.
void GCode::print_machine_envelope(GCodeOutputStream &file, Print &print)
{
    const GCodeFlavor flavor = print.config().gcode_flavor.value;
    if ( (flavor == gcfMarlinLegacy || flavor == gcfMarlinFirmware || flavor == gcfRepRapFirmware)
     && print.config().machine_limits_usage.value == MachineLimitsUsage::EmitToGCode) {
        int factor = flavor == gcfRepRapFirmware ? 60 : 1; // RRF M203 and M566 are in mm/min
        file.write_format("M201 X%d Y%d Z%d E%d ; sets maximum accelerations, mm/sec^2\n",
            int(print.config().machine_max_acceleration_x.values.front() + 0.5),
            int(print.config().machine_max_acceleration_y.values.front() + 0.5),
            int(print.config().machine_max_acceleration_z.values.front() + 0.5),
            int(print.config().machine_max_acceleration_e.values.front() + 0.5));
        file.write_format("M203 X%d Y%d Z%d E%d ; sets maximum feedrates, %s\n",
            int(print.config().machine_max_feedrate_x.values.front() * factor + 0.5),
            int(print.config().machine_max_feedrate_y.values.front() * factor + 0.5),
            int(print.config().machine_max_feedrate_z.values.front() * factor + 0.5),
            int(print.config().machine_max_feedrate_e.values.front() * factor + 0.5),
            factor == 60 ? "mm / min" : "mm / sec");

        // Now M204 - acceleration. This one is quite hairy...
        if (flavor == gcfRepRapFirmware)
            // Uses M204 P[print] T[travel]
            file.write_format("M204 P%d T%d ; sets acceleration (P, T), mm/sec^2\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                int(print.config().machine_max_acceleration_travel.values.front() + 0.5));
        else if (flavor == gcfMarlinLegacy)
            // Legacy Marlin uses M204 S[print] T[retract]
            file.write_format("M204 S%d T%d ; sets acceleration (S) and retract acceleration (R), mm/sec^2\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                int(print.config().machine_max_acceleration_retracting.values.front() + 0.5));
        else if (flavor == gcfMarlinFirmware)
            // New Marlin uses M204 P[print] R[retract] T[travel]
            file.write_format("M204 P%d R%d T%d ; sets acceleration (P, T) and retract acceleration (R), mm/sec^2\n",
                int(print.config().machine_max_acceleration_extruding.values.front() + 0.5),
                int(print.config().machine_max_acceleration_retracting.values.front() + 0.5),
                int(print.config().machine_max_acceleration_travel.values.front() + 0.5));
        else
            assert(false);

        assert(is_decimal_separator_point());
        file.write_format(flavor == gcfRepRapFirmware
            ? "M566 X%.2lf Y%.2lf Z%.2lf E%.2lf ; sets the jerk limits, mm/min\n"
            : "M205 X%.2lf Y%.2lf Z%.2lf E%.2lf ; sets the jerk limits, mm/sec\n",
            print.config().machine_max_jerk_x.values.front() * factor,
            print.config().machine_max_jerk_y.values.front() * factor,
            print.config().machine_max_jerk_z.values.front() * factor,
            print.config().machine_max_jerk_e.values.front() * factor);
        if (flavor != gcfRepRapFirmware)
            file.write_format("M205 S%d T%d ; sets the minimum extruding and travel feed rate, mm/sec\n",
                int(print.config().machine_min_extruding_rate.values.front() + 0.5),
                int(print.config().machine_min_travel_rate.values.front() + 0.5));
        else {
            // M205 Sn Tn not supported in RRF. They use M203 Inn to set minimum feedrate for
            // all moves. This is currently not implemented.
        }
    }
}

// Write 1st layer bed temperatures into the G-code.
// Only do that if the start G-code does not already contain any M-code controlling an extruder temperature.
// M140 - Set Extruder Temperature
// M190 - Set Extruder Temperature and Wait
void GCode::_print_first_layer_bed_temperature(GCodeOutputStream &file, Print &print, const std::string &gcode, unsigned int first_printing_extruder_id, bool wait)
{
    bool autoemit = print.config().autoemit_temperature_commands;
    // Initial bed temperature based on the first extruder.
    int  temp = print.config().first_layer_bed_temperature.get_at(first_printing_extruder_id);
    // Is the bed temperature set by the provided custom G-code?
    int  temp_by_gcode     = -1;
    bool temp_set_by_gcode = custom_gcode_sets_temperature(gcode, 140, 190, false, temp_by_gcode);
    if (autoemit && temp_set_by_gcode && temp_by_gcode >= 0 && temp_by_gcode < 1000)
        temp = temp_by_gcode;
    // Always call m_writer.set_bed_temperature() so it will set the internal "current" state of the bed temp as if
    // the custom start G-code emited these.
    std::string set_temp_gcode = m_writer.set_bed_temperature(temp, wait);
    if (autoemit && ! temp_set_by_gcode)
        file.write(set_temp_gcode);
}

// Write 1st layer extruder temperatures into the G-code.
// Only do that if the start G-code does not already contain any M-code controlling an extruder temperature.
// M104 - Set Extruder Temperature
// M109 - Set Extruder Temperature and Wait
// RepRapFirmware: G10 Sxx
void GCode::_print_first_layer_extruder_temperatures(GCodeOutputStream &file, Print &print, const std::string &gcode, unsigned int first_printing_extruder_id, bool wait)
{
    bool autoemit = print.config().autoemit_temperature_commands;
    // Is the bed temperature set by the provided custom G-code?
    int  temp_by_gcode = -1;
    bool include_g10   = print.config().gcode_flavor == gcfRepRapFirmware;
    if (! autoemit  || custom_gcode_sets_temperature(gcode, 104, 109, include_g10, temp_by_gcode)) {
        // Set the extruder temperature at m_writer, but throw away the generated G-code as it will be written with the custom G-code.
        int temp = print.config().first_layer_temperature.get_at(first_printing_extruder_id);
        if (autoemit && temp_by_gcode >= 0 && temp_by_gcode < 1000)
            temp = temp_by_gcode;
        m_writer.set_temperature(temp, wait, first_printing_extruder_id);
    } else {
        // Custom G-code does not set the extruder temperature. Do it now.
        if (print.config().single_extruder_multi_material.value) {
            // Set temperature of the first printing extruder only.
            int temp = print.config().first_layer_temperature.get_at(first_printing_extruder_id);
            if (temp > 0)
                file.write(m_writer.set_temperature(temp, wait, first_printing_extruder_id));
        } else {
            // Set temperatures of all the printing extruders.
            for (unsigned int tool_id : print.extruders()) {
                int temp = print.config().first_layer_temperature.get_at(tool_id);

                if (print.config().ooze_prevention.value && tool_id != first_printing_extruder_id) {
                    if (print.config().idle_temperature.is_nil(tool_id))
                        temp += print.config().standby_temperature_delta.value;
                    else
                        temp = print.config().idle_temperature.get_at(tool_id);
                }

                if (temp > 0)
                    file.write(m_writer.set_temperature(temp, wait, tool_id));
            }
        }
    }
}

std::vector<GCode::InstanceToPrint> GCode::sort_print_object_instances(
    const std::vector<ObjectLayerToPrint>       &object_layers,
    // Ordering must be defined for normal (non-sequential print).
    const std::vector<const PrintInstance*>     *ordering,
    // For sequential print, the instance of the object to be printing has to be defined.
    const size_t                                 single_object_instance_idx)
{
    std::vector<InstanceToPrint> out;

    if (ordering == nullptr) {
        // Sequential print, single object is being printed.
        assert(object_layers.size() == 1);
        out.emplace_back(0, *object_layers.front().object(), single_object_instance_idx);
    } else {
        // Create mapping from PrintObject* to ObjectLayerToPrint ID.
        std::vector<std::pair<const PrintObject*, size_t>> sorted;
        sorted.reserve(object_layers.size());
        for (const ObjectLayerToPrint &object : object_layers)
            if (const PrintObject* print_object = object.object(); print_object)
                sorted.emplace_back(print_object, &object - object_layers.data());
        std::sort(sorted.begin(), sorted.end());

        if (! sorted.empty()) {
            out.reserve(sorted.size());
            for (const PrintInstance *instance : *ordering) {
                const PrintObject &print_object = *instance->print_object;
                std::pair<const PrintObject*, size_t> key(&print_object, 0);
                auto it = std::lower_bound(sorted.begin(), sorted.end(), key);
                if (it != sorted.end() && it->first == &print_object)
                    // ObjectLayerToPrint for this PrintObject was found.
                    out.emplace_back(it->second, print_object, instance - print_object.instances().data());
            }
        }
    }
    return out;
}

namespace ProcessLayer
{

    static std::string emit_custom_gcode_per_print_z(
        GCode                                                   &gcodegen,
        const CustomGCode::Item 								*custom_gcode,
        unsigned int                                             current_extruder_id,
        // ID of the first extruder printing this layer.
        unsigned int                                             first_extruder_id,
        const PrintConfig                                       &config)
    {
        std::string gcode;
        bool single_extruder_printer = config.nozzle_diameter.size() == 1;

        if (custom_gcode != nullptr) {
            // Extruder switches are processed by LayerTools, they should be filtered out.
            assert(custom_gcode->type != CustomGCode::ToolChange);

            CustomGCode::Type   gcode_type   = custom_gcode->type;
            bool  				color_change = gcode_type == CustomGCode::ColorChange;
            bool 				tool_change  = gcode_type == CustomGCode::ToolChange;
            // Tool Change is applied as Color Change for a single extruder printer only.
            assert(! tool_change || single_extruder_printer);

            std::string pause_print_msg;
            int m600_extruder_before_layer = -1;
            if (color_change && custom_gcode->extruder > 0)
                m600_extruder_before_layer = custom_gcode->extruder - 1;
            else if (gcode_type == CustomGCode::PausePrint)
                pause_print_msg = custom_gcode->extra;

            // we should add or not colorprint_change in respect to nozzle_diameter count instead of really used extruders count
            if (color_change || tool_change)
            {
                assert(m600_extruder_before_layer >= 0);
		        // Color Change or Tool Change as Color Change.
                // add tag for processor
                gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Color_Change) + ",T" + std::to_string(m600_extruder_before_layer) + "," + custom_gcode->color + "\n";

                if (!single_extruder_printer && m600_extruder_before_layer >= 0 && first_extruder_id != (unsigned)m600_extruder_before_layer
                    // && !MMU1
                    ) {
                    //! FIXME_in_fw show message during print pause
                    // FIXME: Why is pause_print_gcode here? Why is it supplied "color_change_extruder"? Why is that not 
                    //        passed to color_change_gcode below?
                    DynamicConfig cfg;
                    cfg.set_key_value("color_change_extruder", new ConfigOptionInt(m600_extruder_before_layer));
                    gcode += gcodegen.placeholder_parser_process("pause_print_gcode", config.pause_print_gcode, current_extruder_id, &cfg);
                    gcode += "\n";
                    gcode += "M117 Change filament for Extruder " + std::to_string(m600_extruder_before_layer) + "\n";
                }
                else {
                    gcode += gcodegen.placeholder_parser_process("color_change_gcode", config.color_change_gcode, current_extruder_id);
                    gcode += "\n";
                    //FIXME Tell G-code writer that M600 filled the extruder, thus the G-code writer shall reset the extruder to unretracted state after
                    // return from M600. Thus the G-code generated by the following line is ignored.
                    // see GH issue #6362
                    gcodegen.writer().unretract();
                }
	        } 
	        else {
	            if (gcode_type == CustomGCode::PausePrint) // Pause print
	            {
                    // add tag for processor
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Pause_Print) + "\n";
                    //! FIXME_in_fw show message during print pause
	                if (!pause_print_msg.empty())
	                    gcode += "M117 " + pause_print_msg + "\n";
                    gcode += gcodegen.placeholder_parser_process("pause_print_gcode", config.pause_print_gcode, current_extruder_id);
                }
	            else {
                    // add tag for processor
                    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Custom_Code) + "\n";
                    if (gcode_type == CustomGCode::Template)    // Template Custom Gcode
                        gcode += gcodegen.placeholder_parser_process("template_custom_gcode", config.template_custom_gcode, current_extruder_id);
                    else                                        // custom Gcode
                        gcode += custom_gcode->extra;

                }
                gcode += "\n";
            }
        }

        return gcode;
    }
} // namespace ProcessLayer

namespace Skirt {
    static void skirt_loops_per_extruder_all_printing(const Print &print, const LayerTools &layer_tools, std::map<unsigned int, std::pair<size_t, size_t>> &skirt_loops_per_extruder_out)
    {
        // Prime all extruders printing over the 1st layer over the skirt lines.
        size_t n_loops = print.skirt().entities.size();
        size_t n_tools = layer_tools.extruders.size();
        size_t lines_per_extruder = (n_loops + n_tools - 1) / n_tools;
        for (size_t i = 0; i < n_loops; i += lines_per_extruder)
            skirt_loops_per_extruder_out[layer_tools.extruders[i / lines_per_extruder]] = std::pair<size_t, size_t>(i, std::min(i + lines_per_extruder, n_loops));
    }

    static std::map<unsigned int, std::pair<size_t, size_t>> make_skirt_loops_per_extruder_1st_layer(
        const Print             				&print,
        const LayerTools                		&layer_tools,
        // Heights (print_z) at which the skirt has already been extruded.
        std::vector<coordf_t>  			    	&skirt_done)
    {
        // Extrude skirt at the print_z of the raft layers and normal object layers
        // not at the print_z of the interlaced support material layers.
        std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder_out;
        //For sequential print, the following test may fail when extruding the 2nd and other objects.
        // assert(skirt_done.empty());
        if (skirt_done.empty() && print.has_skirt() && ! print.skirt().entities.empty() && layer_tools.has_skirt) {
            skirt_loops_per_extruder_all_printing(print, layer_tools, skirt_loops_per_extruder_out);
            skirt_done.emplace_back(layer_tools.print_z);
        }
        return skirt_loops_per_extruder_out;
    }

    static std::map<unsigned int, std::pair<size_t, size_t>> make_skirt_loops_per_extruder_other_layers(
        const Print 							&print,
        const LayerTools                		&layer_tools,
        // Heights (print_z) at which the skirt has already been extruded.
        std::vector<coordf_t>			    	&skirt_done)
    {
        // Extrude skirt at the print_z of the raft layers and normal object layers
        // not at the print_z of the interlaced support material layers.
        std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder_out;
        if (print.has_skirt() && ! print.skirt().entities.empty() && layer_tools.has_skirt &&
            // Not enough skirt layers printed yet.
            //FIXME infinite or high skirt does not make sense for sequential print!
            (skirt_done.size() < (size_t)print.config().skirt_height.value || print.has_infinite_skirt())) {
            bool valid = ! skirt_done.empty() && skirt_done.back() < layer_tools.print_z - EPSILON;
            assert(valid);
            // This print_z has not been extruded yet (sequential print)
            // FIXME: The skirt_done should not be empty at this point. The check is a workaround
            // of https://github.com/prusa3d/PrusaSlicer/issues/5652, but it deserves a real fix.
            if (valid) {
#if 0
                // Prime just the first printing extruder. This is original Slic3r's implementation.
                skirt_loops_per_extruder_out[layer_tools.extruders.front()] = std::pair<size_t, size_t>(0, print.config().skirts.value);
#else
                // Prime all extruders planned for this layer, see
                // https://github.com/prusa3d/PrusaSlicer/issues/469#issuecomment-322450619
                skirt_loops_per_extruder_all_printing(print, layer_tools, skirt_loops_per_extruder_out);
#endif
                assert(!skirt_done.empty());
                skirt_done.emplace_back(layer_tools.print_z);
            }
        }
        return skirt_loops_per_extruder_out;
    }

} // namespace Skirt

// In sequential mode, process_layer is called once per each object and its copy,
// therefore layers will contain a single entry and single_object_instance_idx will point to the copy of the object.
// In non-sequential mode, process_layer is called per each print_z height with all object and support layers accumulated.
// For multi-material prints, this routine minimizes extruder switches by gathering extruder specific extrusion paths
// and performing the extruder specific extrusions together.
LayerResult GCode::process_layer(
    const Print                    			&print,
    // Set of object & print layers of the same PrintObject and with the same print_z.
    const ObjectsLayerToPrint           	&layers,
    const LayerTools        		        &layer_tools,
    const bool                               last_layer,
    // Pairs of PrintObject index and its instance index.
    const std::vector<const PrintInstance*> *ordering,
    // If set to size_t(-1), then print all copies of all objects.
    // Otherwise print a single copy of a single object.
    const size_t                     		 single_object_instance_idx)
{
    assert(! layers.empty());
    // Either printing all copies of all objects, or just a single copy of a single object.
    assert(single_object_instance_idx == size_t(-1) || layers.size() == 1);

    // First object, support and raft layer, if available.
    const Layer         *object_layer  = nullptr;
    const SupportLayer  *support_layer = nullptr;
    const SupportLayer  *raft_layer    = nullptr;
    for (const ObjectLayerToPrint &l : layers) {
        if (l.object_layer && ! object_layer)
            object_layer = l.object_layer;
        if (l.support_layer) {
            if (! support_layer)
                support_layer = l.support_layer;
            if (! raft_layer && support_layer->id() < support_layer->object()->slicing_parameters().raft_layers())
                raft_layer = support_layer;
        }
    }
    const Layer  &layer = (object_layer != nullptr) ? *object_layer : *support_layer;
    LayerResult   result { {}, layer.id(), false, last_layer, false};
    if (layer_tools.extruders.empty())
        // Nothing to extrude.
        return result;

    // Extract 1st object_layer and support_layer of this set of layers with an equal print_z.
    coordf_t             print_z       = layer.print_z;
    bool                 first_layer   = layer.id() == 0;
    unsigned int         first_extruder_id = layer_tools.extruders.front();

    // Initialize config with the 1st object to be printed at this layer.
    m_config.apply(layer.object()->config(), true);

    // Check whether it is possible to apply the spiral vase logic for this layer.
    // Just a reminder: A spiral vase mode is allowed for a single object, single material print only.
    m_enable_loop_clipping = true;
    if (m_spiral_vase && layers.size() == 1 && support_layer == nullptr) {
        bool enable = (layer.id() > 0 || !print.has_brim()) && (layer.id() >= (size_t)print.config().skirt_height.value && ! print.has_infinite_skirt());
        if (enable) {
            for (const LayerRegion *layer_region : layer.regions())
                if (size_t(layer_region->region().config().bottom_solid_layers.value) > layer.id() ||
                    layer_region->perimeters().items_count() > 1u ||
                    layer_region->fills().items_count() > 0) {
                    enable = false;
                    break;
                }
        }
        result.spiral_vase_enable = enable;
        // If we're going to apply spiralvase to this layer, disable loop clipping.
        m_enable_loop_clipping = !enable;
    }

    std::string gcode;
    assert(is_decimal_separator_point()); // for the sprintfs

    // add tag for processor
    gcode += ";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Layer_Change) + "\n";
    // export layer z
    gcode += std::string(";Z:") + float_to_string_decimal_point(print_z) + "\n";

    // export layer height
    float height = first_layer ? static_cast<float>(print_z) : static_cast<float>(print_z) - m_last_layer_z;
    gcode += std::string(";") + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Height)
        + float_to_string_decimal_point(height) + "\n";

    // update caches
    m_last_layer_z = static_cast<float>(print_z);
    m_max_layer_z  = std::max(m_max_layer_z, m_last_layer_z);
    m_last_height = height;

    // Set new layer - this will change Z and force a retraction if retract_layer_change is enabled.
    if (! print.config().before_layer_gcode.value.empty()) {
        DynamicConfig config;
        config.set_key_value("layer_num",   new ConfigOptionInt(m_layer_index + 1));
        config.set_key_value("layer_z",     new ConfigOptionFloat(print_z));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        gcode += this->placeholder_parser_process("before_layer_gcode",
            print.config().before_layer_gcode.value, m_writer.extruder()->id(), &config)
            + "\n";
    }
    gcode += this->change_layer(print_z);  // this will increase m_layer_index
    m_layer = &layer;
    m_object_layer_over_raft = false;
    if (! print.config().layer_gcode.value.empty()) {
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z",   new ConfigOptionFloat(print_z));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        gcode += this->placeholder_parser_process("layer_gcode",
            print.config().layer_gcode.value, m_writer.extruder()->id(), &config)
            + "\n";
    }

    if (! first_layer && ! m_second_layer_things_done) {
        // Transition from 1st to 2nd layer. Adjust nozzle temperatures as prescribed by the nozzle dependent
        // first_layer_temperature vs. temperature settings.
        for (const Extruder &extruder : m_writer.extruders()) {
            if (print.config().single_extruder_multi_material.value || m_ooze_prevention.enable) {
                // In single extruder multi material mode, set the temperature for the current extruder only.
                // The same applies when ooze prevention is enabled.
                if (extruder.id() != m_writer.extruder()->id())
                    continue;
            }
            int temperature = print.config().temperature.get_at(extruder.id());
            if (temperature > 0 && (temperature != print.config().first_layer_temperature.get_at(extruder.id())))
                gcode += m_writer.set_temperature(temperature, false, extruder.id());
        }
        gcode += m_writer.set_bed_temperature(print.config().bed_temperature.get_at(first_extruder_id));
        // Mark the temperature transition from 1st to 2nd layer to be finished.
        m_second_layer_things_done = true;
    }

    // Map from extruder ID to <begin, end> index of skirt loops to be extruded with that extruder.
    std::map<unsigned int, std::pair<size_t, size_t>> skirt_loops_per_extruder;

    if (single_object_instance_idx == size_t(-1)) {
        // Normal (non-sequential) print.
        gcode += ProcessLayer::emit_custom_gcode_per_print_z(*this, layer_tools.custom_gcode, m_writer.extruder()->id(), first_extruder_id, print.config());
    }
    // Extrude skirt at the print_z of the raft layers and normal object layers
    // not at the print_z of the interlaced support material layers.
    skirt_loops_per_extruder = first_layer ?
        Skirt::make_skirt_loops_per_extruder_1st_layer(print, layer_tools, m_skirt_done) :
        Skirt::make_skirt_loops_per_extruder_other_layers(print, layer_tools, m_skirt_done);

    if (this->config().avoid_crossing_curled_overhangs) {
        m_avoid_crossing_curled_overhangs.clear();
        for (const ObjectLayerToPrint &layer_to_print : layers) {
            if (layer_to_print.object() == nullptr)
                continue;
            for (const auto &instance : layer_to_print.object()->instances()) {
                m_avoid_crossing_curled_overhangs.add_obstacles(layer_to_print.object_layer, instance.shift);
                m_avoid_crossing_curled_overhangs.add_obstacles(layer_to_print.support_layer, instance.shift);
            }
        }
    }

    for (const ObjectLayerToPrint &layer_to_print : layers) {
        m_extrusion_quality_estimator.prepare_for_new_layer(layer_to_print.object_layer);
    }

    // Extrude the skirt, brim, support, perimeters, infill ordered by the extruders.
    for (unsigned int extruder_id : layer_tools.extruders)
    {
        gcode += (layer_tools.has_wipe_tower && m_wipe_tower) ?
            m_wipe_tower->tool_change(*this, extruder_id, extruder_id == layer_tools.extruders.back()) :
            this->set_extruder(extruder_id, print_z);

        // let analyzer tag generator aware of a role type change
        if (layer_tools.has_wipe_tower && m_wipe_tower)
            m_last_processor_extrusion_role = GCodeExtrusionRole::WipeTower;

        if (auto loops_it = skirt_loops_per_extruder.find(extruder_id); loops_it != skirt_loops_per_extruder.end()) {
            const std::pair<size_t, size_t> loops = loops_it->second;
            this->set_origin(0., 0.);
            m_avoid_crossing_perimeters.use_external_mp();
            Flow layer_skirt_flow = print.skirt_flow().with_height(float(m_skirt_done.back() - (m_skirt_done.size() == 1 ? 0. : m_skirt_done[m_skirt_done.size() - 2])));
            double mm3_per_mm = layer_skirt_flow.mm3_per_mm();
            for (size_t i = loops.first; i < loops.second; ++i) {
                // Adjust flow according to this layer's layer height.
                ExtrusionLoop loop = *dynamic_cast<const ExtrusionLoop*>(print.skirt().entities[i]);
                for (ExtrusionPath &path : loop.paths) {
                    path.height = layer_skirt_flow.height();
                    path.mm3_per_mm = mm3_per_mm;
                }
                //FIXME using the support_material_speed of the 1st object printed.
                gcode += this->extrude_loop(loop, "skirt"sv, m_config.support_material_speed.value);
            }
            m_avoid_crossing_perimeters.use_external_mp(false);
            // Allow a straight travel move to the first object point if this is the first layer (but don't in next layers).
            if (first_layer && loops.first == 0)
                m_avoid_crossing_perimeters.disable_once();
        }

        // Extrude brim with the extruder of the 1st region.
        if (! m_brim_done) {
            this->set_origin(0., 0.);
            m_avoid_crossing_perimeters.use_external_mp();
            for (const ExtrusionEntity *ee : print.brim().entities) {
                gcode += this->extrude_entity(*ee, "brim"sv, m_config.support_material_speed.value);
            }
            m_brim_done = true;
            m_avoid_crossing_perimeters.use_external_mp(false);
            // Allow a straight travel move to the first object point.
            m_avoid_crossing_perimeters.disable_once();
        }

        std::vector<InstanceToPrint> instances_to_print = sort_print_object_instances(layers, ordering, single_object_instance_idx);

        // We are almost ready to print. However, we must go through all the objects twice to print the the overridden extrusions first (infill/perimeter wiping feature):
        bool is_anything_overridden = layer_tools.wiping_extrusions().is_anything_overridden();
        if (is_anything_overridden) {
            // Extrude wipes.
            size_t gcode_size_old = gcode.size();
            for (const InstanceToPrint &instance : instances_to_print)
                this->process_layer_single_object(
                    gcode, extruder_id, instance,
                    layers[instance.object_layer_to_print_id], layer_tools,
                    is_anything_overridden, true /* print_wipe_extrusions */);
            if (gcode_size_old < gcode.size())
                gcode+="; PURGING FINISHED\n";
        }
        // Extrude normal extrusions.
        for (const InstanceToPrint &instance : instances_to_print)
            this->process_layer_single_object(
                gcode, extruder_id, instance,
                layers[instance.object_layer_to_print_id], layer_tools,
                is_anything_overridden, false /* print_wipe_extrusions */);
    }

    BOOST_LOG_TRIVIAL(trace) << "Exported layer " << layer.id() << " print_z " << print_z <<
    log_memory_info();

    result.gcode = std::move(gcode);
    result.cooling_buffer_flush = object_layer || raft_layer || last_layer;
    return result;
}

static const auto comment_perimeter = "perimeter"sv;
// Comparing string_view pointer & length for speed.
static inline bool comment_is_perimeter(const std::string_view comment) {
    return comment.data() == comment_perimeter.data() && comment.size() == comment_perimeter.size();
}

void GCode::process_layer_single_object(
    // output
    std::string              &gcode, 
    // Index of the extruder currently active.
    const unsigned int        extruder_id,
    // What object and instance is going to be printed.
    const InstanceToPrint    &print_instance,
    // and the object & support layer of the above.
    const ObjectLayerToPrint &layer_to_print, 
    // Container for extruder overrides (when wiping into object or infill).
    const LayerTools         &layer_tools,
    // Is any extrusion possibly marked as wiping extrusion?
    const bool                is_anything_overridden, 
    // Round 1 (wiping into object or infill) or round 2 (normal extrusions).
    const bool                print_wipe_extrusions)
{
    bool     first     = true;
    int      object_id = 0;
    // Delay layer initialization as many layers may not print with all extruders.
    auto init_layer_delayed = [this, &print_instance, &layer_to_print, &first, &object_id, &gcode]() {
        if (first) {
            first = false;
            const PrintObject &print_object = print_instance.print_object;
            const Print       &print        = *print_object.print();
            m_config.apply(print_object.config(), true);
            m_layer = layer_to_print.layer();
            if (print.config().avoid_crossing_perimeters)
                m_avoid_crossing_perimeters.init_layer(*m_layer);
            // When starting a new object, use the external motion planner for the first travel move.
            const Point &offset = print_object.instances()[print_instance.instance_id].shift;
            std::pair<const PrintObject*, Point> this_object_copy(&print_object, offset);
            if (m_last_obj_copy != this_object_copy)
                m_avoid_crossing_perimeters.use_external_mp_once();
            m_last_obj_copy = this_object_copy;
            this->set_origin(unscale(offset));
            if (this->config().gcode_label_objects) {
                for (const PrintObject *po : print_object.print()->objects())
                    if (po == &print_object)
                        break;
                    else
                        ++ object_id;
                if (config().gcode_flavor == gcfKlipper) {
                    gcode += std::string("EXCLUDE_OBJECT_START NAME=") + make_klipper_exclude_object_name(print_object.model_object()->name, object_id, print_instance.instance_id) + "\n";
                } else
                  gcode += std::string("; printing object ") + print_object.model_object()->name + " id:" + std::to_string(object_id) + " copy " + std::to_string(print_instance.instance_id) + "\n";
            }
        }
    };

    const PrintObject &print_object = print_instance.print_object;
    const Print       &print        = *print_object.print();

    m_extrusion_quality_estimator.set_current_object(&print_object);

    if (! print_wipe_extrusions && layer_to_print.support_layer != nullptr)
        if (const SupportLayer &support_layer = *layer_to_print.support_layer; ! support_layer.support_fills.entities.empty()) {
            ExtrusionRole   role               = support_layer.support_fills.role();
            bool            has_support        = role.is_mixed() || role.is_support_base();
            bool            has_interface      = role.is_mixed() || role.is_support_interface();
            // Extruder ID of the support base. -1 if "don't care".
            unsigned int    support_extruder   = print_object.config().support_material_extruder.value - 1;
            // Shall the support be printed with the active extruder, preferably with non-soluble, to avoid tool changes?
            bool            support_dontcare   = support_extruder == std::numeric_limits<unsigned int>::max();
            // Extruder ID of the support interface. -1 if "don't care".
            unsigned int    interface_extruder = print_object.config().support_material_interface_extruder.value - 1;
            // Shall the support interface be printed with the active extruder, preferably with non-soluble, to avoid tool changes?
            bool            interface_dontcare = interface_extruder == std::numeric_limits<unsigned int>::max();
            if (support_dontcare || interface_dontcare) {
                // Some support will be printed with "don't care" material, preferably non-soluble.
                // Is the current extruder assigned a soluble filament?
                auto it_nonsoluble = std::find_if(layer_tools.extruders.begin(), layer_tools.extruders.end(), 
                    [&soluble = std::as_const(print.config().filament_soluble)](unsigned int extruder_id) { return ! soluble.get_at(extruder_id); });
                // There should be a non-soluble extruder available.
                assert(it_nonsoluble != layer_tools.extruders.end());
                unsigned int dontcare_extruder = it_nonsoluble == layer_tools.extruders.end() ? layer_tools.extruders.front() : *it_nonsoluble;
                if (support_dontcare)
                    support_extruder = dontcare_extruder;
                if (interface_dontcare)
                    interface_extruder = dontcare_extruder;
            }
            bool extrude_support   = has_support && support_extruder == extruder_id;
            bool extrude_interface = has_interface && interface_extruder == extruder_id;
            if (extrude_support || extrude_interface) {
                init_layer_delayed();
                m_layer = layer_to_print.support_layer;
                m_object_layer_over_raft = false;
                gcode += this->extrude_support(
                    // support_extrusion_role is ExtrusionRole::SupportMaterial, ExtrusionRole::SupportMaterialInterface or ExtrusionRole::Mixed for all extrusion paths.
                    support_layer.support_fills.chained_path_from(m_last_pos, extrude_support ? (extrude_interface ? ExtrusionRole::Mixed : ExtrusionRole::SupportMaterial) : ExtrusionRole::SupportMaterialInterface));
            }
        }

    m_layer = layer_to_print.layer();
    // To control print speed of the 1st object layer printed over raft interface.
    m_object_layer_over_raft = layer_to_print.object_layer && layer_to_print.object_layer->id() > 0 &&
        print_object.slicing_parameters().raft_layers() == layer_to_print.object_layer->id();

    // Check whether this ExtrusionEntityCollection should be printed now with extruder_id, given print_wipe_extrusions
    // (wipe extrusions are printed before regular extrusions).
    auto shall_print_this_extrusion_collection = [extruder_id, instance_id = print_instance.instance_id, &layer_tools, is_anything_overridden, print_wipe_extrusions](const ExtrusionEntityCollection *eec, const PrintRegion &region) -> bool {
        assert(eec != nullptr);
        if (eec->entities.empty())
            // This shouldn't happen. FIXME why? but first_point() would fail.
            return false;
        // This extrusion is part of certain Region, which tells us which extruder should be used for it:
        int correct_extruder_id = layer_tools.extruder(*eec, region);
        if (! layer_tools.has_extruder(correct_extruder_id)) {
            // this entity is not overridden, but its extruder is not in layer_tools - we'll print it
            // by last extruder on this layer (could happen e.g. when a wiping object is taller than others - dontcare extruders are eradicated from layer_tools)
            correct_extruder_id = layer_tools.extruders.back();
        }
        int extruder_override_id = is_anything_overridden ? layer_tools.wiping_extrusions().get_extruder_override(eec, instance_id) : -1;
        return print_wipe_extrusions ?
            extruder_override_id == int(extruder_id) :
            extruder_override_id < 0 && int(extruder_id) == correct_extruder_id;
    };

    ExtrusionEntitiesPtr temp_fill_extrusions;
    if (const Layer *layer = layer_to_print.object_layer; layer)
        for (size_t idx : layer->lslice_indices_sorted_by_print_order) {
            const LayerSlice &lslice = layer->lslices_ex[idx];
            auto extrude_infill_range = [&](
                const LayerRegion &layerm, const ExtrusionEntityCollection &fills,
                LayerExtrusionRanges::const_iterator it_fill_ranges_begin, LayerExtrusionRanges::const_iterator it_fill_ranges_end, bool ironing) {
                // PrintObjects own the PrintRegions, thus the pointer to PrintRegion would be unique to a PrintObject, they would not
                // identify the content of PrintRegion accross the whole print uniquely. Translate to a Print specific PrintRegion.
                const PrintRegion &region = print.get_print_region(layerm.region().print_region_id());
                temp_fill_extrusions.clear();
                for (auto it_fill_range = it_fill_ranges_begin; it_fill_range != it_fill_ranges_end; ++ it_fill_range) {
                    assert(it_fill_range->region() == it_fill_ranges_begin->region());
                    for (uint32_t fill_id : *it_fill_range) {
                        assert(dynamic_cast<ExtrusionEntityCollection*>(fills.entities[fill_id]));
                        if (auto *eec = static_cast<ExtrusionEntityCollection*>(fills.entities[fill_id]);
                            (eec->role() == ExtrusionRole::Ironing) == ironing && shall_print_this_extrusion_collection(eec, region)) {
                            if (eec->can_reverse())
                                // Flatten the infill collection for better path planning.
                                for (auto *ee : eec->entities)
                                    temp_fill_extrusions.emplace_back(ee);
                            else
                                temp_fill_extrusions.emplace_back(eec);
                        }
                    }
                }
                if (! temp_fill_extrusions.empty()) {
                    init_layer_delayed();
                    m_config.apply(region.config());
                    //FIXME The source extrusions may be reversed, thus modifying the extrusions! Is it a problem? How about the initial G-code preview?
                    // Will parallel access of initial G-code preview to these extrusions while reordering them at backend cause issues?
                    chain_and_reorder_extrusion_entities(temp_fill_extrusions, &m_last_pos);
                    const auto extrusion_name = ironing ? "ironing"sv : "infill"sv;
                    for (const ExtrusionEntity *fill : temp_fill_extrusions)
                        if (auto *eec = dynamic_cast<const ExtrusionEntityCollection*>(fill); eec) {
                            for (const ExtrusionEntity *ee : eec->chained_path_from(m_last_pos).entities)
                                gcode += this->extrude_entity(*ee, extrusion_name);
                        } else
                            gcode += this->extrude_entity(*fill, extrusion_name);
                }
            };

            //FIXME order islands?
            // Sequential tool path ordering of multiple parts within the same object, aka. perimeter tracking (#5511)
            for (const LayerIsland &island : lslice.islands) {
                auto process_perimeters = [&]() {
                    const LayerRegion &layerm = *layer->get_region(island.perimeters.region());
                    // PrintObjects own the PrintRegions, thus the pointer to PrintRegion would be unique to a PrintObject, they would not
                    // identify the content of PrintRegion accross the whole print uniquely. Translate to a Print specific PrintRegion.
                    const PrintRegion &region = print.get_print_region(layerm.region().print_region_id());
                    bool first = true;
                    for (uint32_t perimeter_id : island.perimeters) {
                        assert(dynamic_cast<const ExtrusionEntityCollection*>(layerm.perimeters().entities[perimeter_id]));
                        if (const auto *eec = static_cast<const ExtrusionEntityCollection*>(layerm.perimeters().entities[perimeter_id]);
                            shall_print_this_extrusion_collection(eec, region)) {
                            // This may not apply to Arachne, but maybe the Arachne gap fill should disable reverse as well?
                            // assert(! eec->can_reverse());
                            if (first) {
                                first = false;
                                init_layer_delayed();
                                m_config.apply(region.config());
                            }
                            for (const ExtrusionEntity *ee : *eec)
                                gcode += this->extrude_entity(*ee, comment_perimeter, -1.);
                        }
                    }
                };
                auto process_infill = [&]() {
                    for (auto it = island.fills.begin(); it != island.fills.end();) {
                        // Gather range of fill ranges with the same region.
                        auto it_end = it;
                        for (++ it_end; it_end != island.fills.end() && it->region() == it_end->region(); ++ it_end) ;
                        const LayerRegion &layerm = *layer->get_region(it->region());
                        extrude_infill_range(layerm, layerm.fills(), it, it_end, false /* normal extrusions, not ironing */);
                        it = it_end;
                    }
                };
                if (print.config().infill_first) {
                    process_infill();
                    process_perimeters();
                } else {
                    process_perimeters();
                    process_infill();
                }
            }
            // ironing
            //FIXME move ironing into the loop above over LayerIslands?
            // First Ironing changes extrusion rate quickly, second single ironing may be done over multiple perimeter regions.
            // Ironing in a second phase is safer, but it may be less efficient.
            for (const LayerIsland &island : lslice.islands) {
                for (auto it = island.fills.begin(); it != island.fills.end();) {
                    // Gather range of fill ranges with the same region.
                    auto it_end = it;
                    for (++ it_end; it_end != island.fills.end() && it->region() == it_end->region(); ++ it_end) ;
                    const LayerRegion &layerm = *layer->get_region(it->region());
                    extrude_infill_range(layerm, layerm.fills(), it, it_end, true /* ironing, not normal extrusions */);
                    it = it_end;
                }
            }
        }
    if (!first && this->config().gcode_label_objects) {
        if (config().gcode_flavor == gcfKlipper) {
            gcode += std::string("EXCLUDE_OBJECT_END NAME=") + make_klipper_exclude_object_name(print_object.model_object()->name, object_id, print_instance.instance_id) + "\n";
        } else
            gcode += std::string("; stop printing object ") + print_object.model_object()->name + " id:" + std::to_string(object_id) + " copy " + std::to_string(print_instance.instance_id) + "\n";
    }
}

void GCode::apply_print_config(const PrintConfig &print_config)
{
    m_writer.apply_print_config(print_config);
    m_config.apply(print_config);
    m_scaled_resolution = scaled<double>(print_config.gcode_resolution.value);
}

void GCode::append_full_config(const Print &print, std::string &str)
{
    const DynamicPrintConfig &cfg = print.full_print_config();
    // Sorted list of config keys, which shall not be stored into the G-code. Initializer list.
    static constexpr auto banned_keys = {
        "compatible_printers"sv,
        "compatible_prints"sv,
        //FIXME The print host keys should not be exported to full_print_config anymore. The following keys may likely be removed.
        "print_host"sv,
        "printhost_apikey"sv,
        "printhost_cafile"sv
    };
    assert(std::is_sorted(banned_keys.begin(), banned_keys.end()));
    auto is_banned = [](const std::string &key) {
        return std::binary_search(banned_keys.begin(), banned_keys.end(), key);
    };
    for (const std::string &key : cfg.keys())
        if (! is_banned(key) && ! cfg.option(key)->is_nil())
            str += "; " + key + " = " + cfg.opt_serialize(key) + "\n";
}

void GCode::set_extruders(const std::vector<unsigned int> &extruder_ids)
{
    m_writer.set_extruders(extruder_ids);

    // enable wipe path generation if any extruder has wipe enabled
    m_wipe.enable = false;
    for (auto id : extruder_ids)
        if (m_config.wipe.get_at(id)) {
            m_wipe.enable = true;
            break;
        }
}

void GCode::set_origin(const Vec2d &pointf)
{
    // if origin increases (goes towards right), last_pos decreases because it goes towards left
    const Point translate(
        scale_(m_origin(0) - pointf(0)),
        scale_(m_origin(1) - pointf(1))
    );
    m_last_pos += translate;
    m_wipe.path.translate(translate);
    m_origin = pointf;
}

std::string GCode::preamble()
{
    std::string gcode = m_writer.preamble();

    /*  Perform a *silent* move to z_offset: we need this to initialize the Z
        position of our writer object so that any initial lift taking place
        before the first layer change will raise the extruder from the correct
        initial Z instead of 0.  */
    m_writer.travel_to_z(m_config.z_offset.value);

    return gcode;
}

// called by GCode::process_layer()
std::string GCode::change_layer(coordf_t print_z)
{
    std::string gcode;
    if (m_layer_count > 0)
        // Increment a progress bar indicator.
        gcode += m_writer.update_progress(++ m_layer_index, m_layer_count);
    coordf_t z = print_z + m_config.z_offset.value;  // in unscaled coordinates
    if (EXTRUDER_CONFIG(retract_layer_change) && m_writer.will_move_z(z))
        gcode += this->retract();

    {
        std::ostringstream comment;
        comment << "move to next layer (" << m_layer_index << ")";
        gcode += m_writer.travel_to_z(z, comment.str());
    }

    // forget last wiping path as wiping after raising Z is pointless
    m_wipe.reset_path();

    return gcode;
}

std::string GCode::extrude_loop(ExtrusionLoop loop, const std::string_view description, double speed)
{
    // get a copy; don't modify the orientation of the original loop object otherwise
    // next copies (if any) would not detect the correct orientation

    // extrude all loops ccw
    bool was_clockwise = loop.make_counter_clockwise();

    // find the point of the loop that is closest to the current extruder position
    // or randomize if requested
    Point last_pos = this->last_pos();

    if (! m_config.spiral_vase && comment_is_perimeter(description)) {
        assert(m_layer != nullptr);
        m_seam_placer.place_seam(m_layer, loop, m_config.external_perimeters_first, this->last_pos());
    } else
        // Because the G-code export has 1um resolution, don't generate segments shorter than 1.5 microns,
        // thus empty path segments will not be produced by G-code export.
        loop.split_at(last_pos, false, scaled<double>(0.0015));

    for (auto it = std::next(loop.paths.begin()); it != loop.paths.end(); ++it) {
        assert(it->polyline.points.size() >= 2);
        assert(std::prev(it)->polyline.last_point() == it->polyline.first_point());
    }
    assert(loop.paths.front().first_point() == loop.paths.back().last_point());

    // clip the path to avoid the extruder to get exactly on the first point of the loop;
    // if polyline was shorter than the clipping distance we'd get a null polyline, so
    // we discard it in that case
    double clip_length = m_enable_loop_clipping ?
        scale_(EXTRUDER_CONFIG(nozzle_diameter)) * LOOP_CLIPPING_LENGTH_OVER_NOZZLE_DIAMETER :
        0;

    // get paths
    ExtrusionPaths paths;
    loop.clip_end(clip_length, &paths);
    if (paths.empty()) return "";

    // apply the small perimeter speed
    if (paths.front().role().is_perimeter() && loop.length() <= SMALL_PERIMETER_LENGTH && speed == -1)
        speed = m_config.small_perimeter_speed.get_abs_value(m_config.perimeter_speed);

    // extrude along the path
    std::string gcode;
    for (ExtrusionPath &path : paths) {
        path.simplify(m_scaled_resolution);
        gcode += this->_extrude(path, description, speed);
    }

    // reset acceleration
    gcode += m_writer.set_print_acceleration((unsigned int)(m_config.default_acceleration.value + 0.5));

    if (m_wipe.enable) {
        m_wipe.path = paths.front().polyline;

        for (auto it = std::next(paths.begin()); it != paths.end(); ++it) {
            if (it->role().is_bridge())
                break; // Don't perform a wipe on bridges.

            assert(it->polyline.points.size() >= 2);
            assert(m_wipe.path.points.back() == it->polyline.first_point());
            if (m_wipe.path.points.back() != it->polyline.first_point())
                break; // ExtrusionLoop is interrupted in some place.

            m_wipe.path.points.insert(m_wipe.path.points.end(), it->polyline.points.begin() + 1, it->polyline.points.end());
        }
    }

    // make a little move inwards before leaving loop
    if (paths.back().role().is_external_perimeter() && m_layer != NULL && m_config.perimeters.value > 1 && paths.front().size() >= 2 && paths.back().polyline.points.size() >= 3) {
        // detect angle between last and first segment
        // the side depends on the original winding order of the polygon (left for contours, right for holes)
        //FIXME improve the algorithm in case the loop is tiny.
        //FIXME improve the algorithm in case the loop is split into segments with a low number of points (see the Point b query).
        // Angle from the 2nd point to the last point.
        double angle_inside = angle(paths.front().polyline.points[1]        - paths.front().first_point(),
                                    *(paths.back().polyline.points.end()-3) - paths.front().first_point());
        assert(angle_inside >= -M_PI && angle_inside <= M_PI);
        // 3rd of this angle will be taken, thus make the angle monotonic before interpolation.
        if (was_clockwise) {
            if (angle_inside > 0)
                angle_inside -= 2.0 * M_PI;
        } else {
            if (angle_inside < 0)
                angle_inside += 2.0 * M_PI;
        }

        // create the destination point along the first segment and rotate it
        // we make sure we don't exceed the segment length because we don't know
        // the rotation of the second segment so we might cross the object boundary
        Vec2d  p1 = paths.front().polyline.points.front().cast<double>();
        Vec2d  p2 = paths.front().polyline.points[1].cast<double>();
        Vec2d  v  = p2 - p1;
        double nd = scale_(EXTRUDER_CONFIG(nozzle_diameter));
        double l2 = v.squaredNorm();
        // Shift by no more than a nozzle diameter.
        //FIXME Hiding the seams will not work nicely for very densely discretized contours!
        Point  pt = ((nd * nd >= l2) ? p2 : (p1 + v * (nd / sqrt(l2)))).cast<coord_t>();
        // Rotate pt inside around the seam point.
        pt.rotate(angle_inside / 3., paths.front().polyline.points.front());
        // generate the travel move
        gcode += m_writer.travel_to_xy(this->point_to_gcode(pt), "move inwards before travel");
    }

    return gcode;
}

std::string GCode::extrude_multi_path(ExtrusionMultiPath multipath, const std::string_view description, double speed)
{
    for (auto it = std::next(multipath.paths.begin()); it != multipath.paths.end(); ++it) {
        assert(it->polyline.points.size() >= 2);
        assert(std::prev(it)->polyline.last_point() == it->polyline.first_point());
    }
    // extrude along the path
    std::string gcode;
    for (ExtrusionPath path : multipath.paths) {
        path.simplify(m_scaled_resolution);
        gcode += this->_extrude(path, description, speed);
    }
    if (m_wipe.enable) {
        m_wipe.path = std::move(multipath.paths.back().polyline);
        m_wipe.path.reverse();

        for (auto it = std::next(multipath.paths.rbegin()); it != multipath.paths.rend(); ++it) {
            if (it->role().is_bridge())
                break; // Do not perform a wipe on bridges.

            assert(it->polyline.points.size() >= 2);
            assert(m_wipe.path.points.back() == it->polyline.last_point());
            if (m_wipe.path.points.back() != it->polyline.last_point())
                break; // ExtrusionMultiPath is interrupted in some place.

            m_wipe.path.points.insert(m_wipe.path.points.end(), it->polyline.points.rbegin() + 1, it->polyline.points.rend());
        }
    }
    // reset acceleration
    gcode += m_writer.set_print_acceleration((unsigned int)floor(m_config.default_acceleration.value + 0.5));
    return gcode;
}

std::string GCode::extrude_entity(const ExtrusionEntity &entity, const std::string_view description, double speed)
{
    if (const ExtrusionPath* path = dynamic_cast<const ExtrusionPath*>(&entity))
        return this->extrude_path(*path, description, speed);
    else if (const ExtrusionMultiPath* multipath = dynamic_cast<const ExtrusionMultiPath*>(&entity))
        return this->extrude_multi_path(*multipath, description, speed);
    else if (const ExtrusionLoop* loop = dynamic_cast<const ExtrusionLoop*>(&entity))
        return this->extrude_loop(*loop, description, speed);
    else
        throw Slic3r::InvalidArgument("Invalid argument supplied to extrude()");
    return "";
}

std::string GCode::extrude_path(ExtrusionPath path, std::string_view description, double speed)
{
    path.simplify(m_scaled_resolution);
    std::string gcode = this->_extrude(path, description, speed);
    if (m_wipe.enable) {
        m_wipe.path = std::move(path.polyline);
        m_wipe.path.reverse();
    }
    // reset acceleration
    gcode += m_writer.set_print_acceleration((unsigned int)floor(m_config.default_acceleration.value + 0.5));
    return gcode;
}

std::string GCode::extrude_support(const ExtrusionEntityCollection &support_fills)
{
    static constexpr const auto support_label            = "support material"sv;
    static constexpr const auto support_interface_label  = "support material interface"sv;

    std::string gcode;
    if (! support_fills.entities.empty()) {
        const double  support_speed            = m_config.support_material_speed.value;
        const double  support_interface_speed  = m_config.support_material_interface_speed.get_abs_value(support_speed);
        for (const ExtrusionEntity *ee : support_fills.entities) {
            ExtrusionRole role = ee->role();
            assert(role == ExtrusionRole::SupportMaterial || role == ExtrusionRole::SupportMaterialInterface);
            const auto   label = (role == ExtrusionRole::SupportMaterial) ? support_label : support_interface_label;
            const double speed = (role == ExtrusionRole::SupportMaterial) ? support_speed : support_interface_speed;
            const ExtrusionPath *path = dynamic_cast<const ExtrusionPath*>(ee);
            if (path)
                gcode += this->extrude_path(*path, label, speed);
            else {
                const ExtrusionMultiPath *multipath = dynamic_cast<const ExtrusionMultiPath*>(ee);
                if (multipath)
                    gcode += this->extrude_multi_path(*multipath, label, speed);
                else {
                    const ExtrusionEntityCollection *eec = dynamic_cast<const ExtrusionEntityCollection*>(ee);
                    assert(eec);
                    if (eec)
                        gcode += this->extrude_support(*eec);
                }
            }
        }
    }
    return gcode;
}

bool GCode::GCodeOutputStream::is_error() const 
{
    return ::ferror(this->f);
}

void GCode::GCodeOutputStream::flush()
{ 
    ::fflush(this->f);
}

void GCode::GCodeOutputStream::close()
{ 
    if (this->f) {
        ::fclose(this->f);
        this->f = nullptr;
    }
}

void GCode::GCodeOutputStream::write(const char *what)
{
    if (what != nullptr) {
        //FIXME don't allocate a string, maybe process a batch of lines?
        std::string gcode(m_find_replace ? m_find_replace->process_layer(what) : what);
        // writes string to file
        fwrite(gcode.c_str(), 1, gcode.size(), this->f);
        m_processor.process_buffer(gcode);
    }
}

void GCode::GCodeOutputStream::writeln(const std::string &what)
{
    if (! what.empty())
        this->write(what.back() == '\n' ? what : what + '\n');
}

void GCode::GCodeOutputStream::write_format(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int buflen;
    {
        va_list args2;
        va_copy(args2, args);
        buflen =
    #ifdef _MSC_VER
            ::_vscprintf(format, args2)
    #else
            ::vsnprintf(nullptr, 0, format, args2)
    #endif
            + 1;
        va_end(args2);
    }

    char buffer[1024];
    bool buffer_dynamic = buflen > 1024;
    char *bufptr = buffer_dynamic ? (char*)malloc(buflen) : buffer;
    int res = ::vsnprintf(bufptr, buflen, format, args);
    if (res > 0)
        this->write(bufptr);

    if (buffer_dynamic)
        free(bufptr);

    va_end(args);
}

std::string GCode::_extrude(const ExtrusionPath &path, const std::string_view description, double speed)
{
    std::string gcode;
    const std::string_view description_bridge = path.role().is_bridge() ? " (bridge)"sv : ""sv;

    // go to first point of extrusion path
    if (!m_last_pos_defined || m_last_pos != path.first_point()) {
        std::string comment = "move to first ";
        comment += description;
        comment += description_bridge;
        comment += " point";
        gcode += this->travel_to(path.first_point(), path.role(), comment);
    }

    // compensate retraction
    gcode += this->unretract();

    // adjust acceleration
    if (m_config.default_acceleration.value > 0) {
        double acceleration;
        if (this->on_first_layer() && m_config.first_layer_acceleration.value > 0) {
            acceleration = m_config.first_layer_acceleration.value;
        } else if (this->object_layer_over_raft() && m_config.first_layer_acceleration_over_raft.value > 0) {
            acceleration = m_config.first_layer_acceleration_over_raft.value;
        } else if (m_config.bridge_acceleration.value > 0 && path.role().is_bridge()) {
            acceleration = m_config.bridge_acceleration.value;
        } else if (m_config.top_solid_infill_acceleration > 0 && path.role() == ExtrusionRole::TopSolidInfill) {
            acceleration = m_config.top_solid_infill_acceleration.value;
        } else if (m_config.solid_infill_acceleration > 0 && path.role().is_solid_infill()) {
            acceleration = m_config.solid_infill_acceleration.value;
        } else if (m_config.infill_acceleration.value > 0 && path.role().is_infill()) {
            acceleration = m_config.infill_acceleration.value;
        } else if (m_config.external_perimeter_acceleration > 0 && path.role().is_external_perimeter()) {
            acceleration = m_config.external_perimeter_acceleration.value;
        } else if (m_config.perimeter_acceleration.value > 0 && path.role().is_perimeter()) {
            acceleration = m_config.perimeter_acceleration.value;
        } else {
            acceleration = m_config.default_acceleration.value;
        }
        gcode += m_writer.set_print_acceleration((unsigned int)floor(acceleration + 0.5));
    }

    // calculate extrusion length per distance unit
    double e_per_mm = m_writer.extruder()->e_per_mm3() * path.mm3_per_mm;
    if (m_writer.extrusion_axis().empty())
        // gcfNoExtrusion
        e_per_mm = 0;

    // set speed
    if (speed == -1) {
        if (path.role() == ExtrusionRole::Perimeter) {
            speed = m_config.get_abs_value("perimeter_speed");
        } else if (path.role() == ExtrusionRole::ExternalPerimeter) {
            speed = m_config.get_abs_value("external_perimeter_speed");
        } else if (path.role().is_bridge()) {
            assert(path.role().is_perimeter() || path.role() == ExtrusionRole::BridgeInfill);
            speed = m_config.get_abs_value("bridge_speed");
        } else if (path.role() == ExtrusionRole::InternalInfill) {
            speed = m_config.get_abs_value("infill_speed");
        } else if (path.role() == ExtrusionRole::SolidInfill) {
            speed = m_config.get_abs_value("solid_infill_speed");
        } else if (path.role() == ExtrusionRole::TopSolidInfill) {
            speed = m_config.get_abs_value("top_solid_infill_speed");
        } else if (path.role() == ExtrusionRole::Ironing) {
            speed = m_config.get_abs_value("ironing_speed");
        } else if (path.role() == ExtrusionRole::GapFill) {
            speed = m_config.get_abs_value("gap_fill_speed");
        } else {
            throw Slic3r::InvalidArgument("Invalid speed");
        }
    }
    if (m_volumetric_speed != 0. && speed == 0)
        speed = m_volumetric_speed / path.mm3_per_mm;
    if (this->on_first_layer())
        speed = m_config.get_abs_value("first_layer_speed", speed);
    else if (this->object_layer_over_raft())
        speed = m_config.get_abs_value("first_layer_speed_over_raft", speed);
    if (m_config.max_volumetric_speed.value > 0) {
        // cap speed with max_volumetric_speed anyway (even if user is not using autospeed)
        speed = std::min(
            speed,
            m_config.max_volumetric_speed.value / path.mm3_per_mm
        );
    }
    if (EXTRUDER_CONFIG(filament_max_volumetric_speed) > 0) {
        // cap speed with max_volumetric_speed anyway (even if user is not using autospeed)
        speed = std::min(
            speed,
            EXTRUDER_CONFIG(filament_max_volumetric_speed) / path.mm3_per_mm
        );
    }

    bool                        variable_speed_or_fan_speed = false;
    std::vector<ProcessedPoint> new_points{};
    if ((this->m_config.enable_dynamic_overhang_speeds || this->config().enable_dynamic_fan_speeds.get_at(m_writer.extruder()->id())) &&
        !this->on_first_layer() && path.role().is_perimeter()) {
        std::vector<std::pair<int, ConfigOptionFloatOrPercent>> overhangs_with_speeds = {{100, ConfigOptionFloatOrPercent{speed, false}}};
        if (this->m_config.enable_dynamic_overhang_speeds) {
            overhangs_with_speeds = {{0, m_config.overhang_speed_0},
                                     {25, m_config.overhang_speed_1},
                                     {50, m_config.overhang_speed_2},
                                     {75, m_config.overhang_speed_3},
                                     {100, ConfigOptionFloatOrPercent{speed, false}}};
        }

        std::vector<std::pair<int, ConfigOptionInts>> overhang_w_fan_speeds = {{100, ConfigOptionInts{0}}};
        if (this->m_config.enable_dynamic_fan_speeds.get_at(m_writer.extruder()->id())) {
            overhang_w_fan_speeds = {{0, m_config.overhang_fan_speed_0},
                                     {25, m_config.overhang_fan_speed_1},
                                     {50, m_config.overhang_fan_speed_2},
                                     {75, m_config.overhang_fan_speed_3},
                                     {100, ConfigOptionInts{0}}};
        }

        double external_perim_reference_speed = m_config.get_abs_value("external_perimeter_speed");
        if (external_perim_reference_speed == 0)
            external_perim_reference_speed = m_volumetric_speed / path.mm3_per_mm;
        if (m_config.max_volumetric_speed.value > 0)
            external_perim_reference_speed = std::min(external_perim_reference_speed, m_config.max_volumetric_speed.value / path.mm3_per_mm);
        if (EXTRUDER_CONFIG(filament_max_volumetric_speed) > 0) {
            external_perim_reference_speed = std::min(external_perim_reference_speed,
                                                      EXTRUDER_CONFIG(filament_max_volumetric_speed) / path.mm3_per_mm);
        }

        new_points = m_extrusion_quality_estimator.estimate_speed_from_extrusion_quality(path, overhangs_with_speeds, overhang_w_fan_speeds,
                                                                              m_writer.extruder()->id(), external_perim_reference_speed,
                                                                              speed);
        variable_speed_or_fan_speed = std::any_of(new_points.begin(), new_points.end(),
                                                  [speed](const ProcessedPoint &p) { return p.speed != speed || p.fan_speed != 0; });
    }

    double F = speed * 60;  // convert mm/sec to mm/min

    // extrude arc or line
    if (m_enable_extrusion_role_markers)
    {
        if (GCodeExtrusionRole role = extrusion_role_to_gcode_extrusion_role(path.role()); role != m_last_extrusion_role)
        {
            m_last_extrusion_role = role;
            if (m_enable_extrusion_role_markers)
            {
                char buf[32];
                sprintf(buf, ";_EXTRUSION_ROLE:%d\n", int(m_last_extrusion_role));
                gcode += buf;
            }
        }
    }

    // adds processor tags and updates processor tracking data
    // PrusaMultiMaterial::Writer may generate GCodeProcessor::Height_Tag lines without updating m_last_height
    // so, if the last role was GCodeExtrusionRole::WipeTower we force export of GCodeProcessor::Height_Tag lines
    bool last_was_wipe_tower = (m_last_processor_extrusion_role == GCodeExtrusionRole::WipeTower);
    assert(is_decimal_separator_point());

    if (GCodeExtrusionRole role = extrusion_role_to_gcode_extrusion_role(path.role()); role != m_last_processor_extrusion_role) {
        m_last_processor_extrusion_role = role;
        char buf[64];
        sprintf(buf, ";%s%s\n", GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role).c_str(), gcode_extrusion_role_to_string(m_last_processor_extrusion_role).c_str());
        gcode += buf;
    }

    if (last_was_wipe_tower || m_last_width != path.width) {
        m_last_width = path.width;
        gcode += std::string(";") + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Width)
               + float_to_string_decimal_point(m_last_width) + "\n";
    }

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    if (last_was_wipe_tower || (m_last_mm3_per_mm != path.mm3_per_mm)) {
        m_last_mm3_per_mm = path.mm3_per_mm;
        gcode += std::string(";") + GCodeProcessor::Mm3_Per_Mm_Tag
            + float_to_string_decimal_point(m_last_mm3_per_mm) + "\n";
    }
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    if (last_was_wipe_tower || std::abs(m_last_height - path.height) > EPSILON) {
        m_last_height = path.height;

        gcode += std::string(";") + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Height)
            + float_to_string_decimal_point(m_last_height) + "\n";
    }

    std::string cooling_marker_setspeed_comments;
    if (m_enable_cooling_markers) {
        if (path.role().is_bridge())
            gcode += ";_BRIDGE_FAN_START\n";
        else
            cooling_marker_setspeed_comments = ";_EXTRUDE_SET_SPEED";
        if (path.role() == ExtrusionRole::ExternalPerimeter)
            cooling_marker_setspeed_comments += ";_EXTERNAL_PERIMETER";
    }

    if (!variable_speed_or_fan_speed) {
        // F is mm per minute.
        gcode += m_writer.set_speed(F, "", cooling_marker_setspeed_comments);
        double path_length = 0.;
        std::string comment;
        if (m_config.gcode_comments) {
            comment = description;
            comment += description_bridge;
        }
        Vec2d prev = this->point_to_gcode_quantized(path.polyline.points.front());
        auto  it   = path.polyline.points.begin();
        auto  end  = path.polyline.points.end();
        for (++ it; it != end; ++ it) {
            Vec2d p = this->point_to_gcode_quantized(*it);
            const double line_length = (p - prev).norm();
            path_length += line_length;
            gcode += m_writer.extrude_to_xy(p, e_per_mm * line_length, comment);
            prev = p;
        }
    } else {
        std::string marked_comment;
        if (m_config.gcode_comments) {
            marked_comment = description;
            marked_comment += description_bridge;
        }
        double last_set_speed     = new_points[0].speed * 60.0;
        double last_set_fan_speed = new_points[0].fan_speed;
        gcode += m_writer.set_speed(last_set_speed, "", cooling_marker_setspeed_comments);
        gcode += "\n;_SET_FAN_SPEED" + std::to_string(int(last_set_fan_speed)) + "\n";
        Vec2d prev = this->point_to_gcode_quantized(new_points[0].p);
        for (size_t i = 1; i < new_points.size(); i++) {
            const ProcessedPoint &processed_point = new_points[i];
            Vec2d                 p               = this->point_to_gcode_quantized(processed_point.p);
            const double          line_length     = (p - prev).norm();
            gcode += m_writer.extrude_to_xy(p, e_per_mm * line_length, marked_comment);
            prev             = p;
            double new_speed = processed_point.speed * 60.0;
            if (last_set_speed != new_speed) {
                gcode += m_writer.set_speed(new_speed, "", cooling_marker_setspeed_comments);
                last_set_speed = new_speed;
            }
            if (last_set_fan_speed != processed_point.fan_speed) {
                last_set_fan_speed = processed_point.fan_speed;
                gcode += "\n;_SET_FAN_SPEED" + std::to_string(int(last_set_fan_speed)) + "\n";
            }
        }
        gcode += "\n;_RESET_FAN_SPEED\n";
    }

    if (m_enable_cooling_markers)
        gcode += path.role().is_bridge() ? ";_BRIDGE_FAN_END\n" : ";_EXTRUDE_END\n";

    this->set_last_pos(path.last_point());
    return gcode;
}

// This method accepts &point in print coordinates.
std::string GCode::travel_to(const Point &point, ExtrusionRole role, std::string comment)
{
    /*  Define the travel move as a line between current position and the taget point.
        This is expressed in print coordinates, so it will need to be translated by
        this->origin in order to get G-code coordinates.  */
    Polyline travel { this->last_pos(), point };

    if (this->config().avoid_crossing_curled_overhangs) {
        if (m_config.avoid_crossing_perimeters) {
            BOOST_LOG_TRIVIAL(warning)
                << "Option >avoid crossing curled overhangs< is not compatible with avoid crossing perimeters and it will be ignored!";
        } else {
            Point scaled_origin = Point(scaled(this->origin()));
            travel              = m_avoid_crossing_curled_overhangs.find_path(this->last_pos() + scaled_origin, point + scaled_origin);
            travel.translate(-scaled_origin);
        }
    }

    // check whether a straight travel move would need retraction
    bool needs_retraction             = this->needs_retraction(travel, role);
    // check whether wipe could be disabled without causing visible stringing
    bool could_be_wipe_disabled       = false;
    // Save state of use_external_mp_once for the case that will be needed to call twice m_avoid_crossing_perimeters.travel_to.
    const bool used_external_mp_once  = m_avoid_crossing_perimeters.used_external_mp_once();

    // if a retraction would be needed, try to use avoid_crossing_perimeters to plan a
    // multi-hop travel path inside the configuration space
    if (needs_retraction
        && m_config.avoid_crossing_perimeters
        && ! m_avoid_crossing_perimeters.disabled_once()) {
        travel = m_avoid_crossing_perimeters.travel_to(*this, point, &could_be_wipe_disabled);
        // check again whether the new travel path still needs a retraction
        needs_retraction = this->needs_retraction(travel, role);
        //if (needs_retraction && m_layer_index > 1) exit(0);
    }

    // Re-allow avoid_crossing_perimeters for the next travel moves
    m_avoid_crossing_perimeters.reset_once_modifiers();

    // generate G-code for the travel move
    std::string gcode;
    if (needs_retraction) {
        if (m_config.avoid_crossing_perimeters && could_be_wipe_disabled)
            m_wipe.reset_path();

        Point last_post_before_retract = this->last_pos();
        gcode += this->retract();
        // When "Wipe while retracting" is enabled, then extruder moves to another position, and travel from this position can cross perimeters.
        // Because of it, it is necessary to call avoid crossing perimeters again with new starting point after calling retraction()
        // FIXME Lukas H.: Try to predict if this second calling of avoid crossing perimeters will be needed or not. It could save computations.
        if (last_post_before_retract != this->last_pos() && m_config.avoid_crossing_perimeters) {
            // If in the previous call of m_avoid_crossing_perimeters.travel_to was use_external_mp_once set to true restore this value for next call.
            if (used_external_mp_once)
                m_avoid_crossing_perimeters.use_external_mp_once();
            travel = m_avoid_crossing_perimeters.travel_to(*this, point);
            // If state of use_external_mp_once was changed reset it to right value.
            if (used_external_mp_once)
                m_avoid_crossing_perimeters.reset_once_modifiers();
        }
    } else
        // Reset the wipe path when traveling, so one would not wipe along an old path.
        m_wipe.reset_path();

    // use G1 because we rely on paths being straight (G0 may make round paths)
    if (travel.size() >= 2) {

        gcode += m_writer.set_travel_acceleration((unsigned int)(m_config.travel_acceleration.value + 0.5));

        for (size_t i = 1; i < travel.size(); ++ i)
            gcode += m_writer.travel_to_xy(this->point_to_gcode(travel.points[i]), comment);

        if (! GCodeWriter::supports_separate_travel_acceleration(config().gcode_flavor)) {
            // In case that this flavor does not support separate print and travel acceleration,
            // reset acceleration to default.
            gcode += m_writer.set_travel_acceleration((unsigned int)(m_config.travel_acceleration.value + 0.5));
        }

        this->set_last_pos(travel.points.back());
    }
    return gcode;
}

bool GCode::needs_retraction(const Polyline &travel, ExtrusionRole role)
{
    if (travel.length() < scale_(EXTRUDER_CONFIG(retract_before_travel))) {
        // skip retraction if the move is shorter than the configured threshold
        return false;
    }

    if (role == ExtrusionRole::SupportMaterial)
        if (const SupportLayer *support_layer = dynamic_cast<const SupportLayer*>(m_layer);
            support_layer != nullptr && ! support_layer->support_islands_bboxes.empty()) {
            BoundingBox bbox_travel = get_extents(travel);
            Polylines   trimmed;
            bool        trimmed_initialized = false;
            for (const BoundingBox &bbox : support_layer->support_islands_bboxes)
                if (bbox.overlap(bbox_travel)) {
                    const auto &island = support_layer->support_islands[&bbox - support_layer->support_islands_bboxes.data()];
                    trimmed = trimmed_initialized ? diff_pl(trimmed, island) : diff_pl(travel, island);
                    trimmed_initialized = true;
                    if (trimmed.empty())
                        // skip retraction if this is a travel move inside a support material island
                        //FIXME not retracting over a long path may cause oozing, which in turn may result in missing material
                        // at the end of the extrusion path!
                        return false;
                    // Not sure whether updating the boudning box isn't too expensive.
                    //bbox_travel = get_extents(trimmed);
                }
        }

    if (m_config.only_retract_when_crossing_perimeters && m_layer != nullptr &&
        m_config.fill_density.value > 0 && m_retract_when_crossing_perimeters.travel_inside_internal_regions(*m_layer, travel))
        // Skip retraction if travel is contained in an internal slice *and*
        // internal infill is enabled (so that stringing is entirely not visible).
        //FIXME any_internal_region_slice_contains() is potentionally very slow, it shall test for the bounding boxes first.
        return false;

    // retract if only_retract_when_crossing_perimeters is disabled or doesn't apply
    return true;
}

std::string GCode::retract(bool toolchange)
{
    std::string gcode;

    if (m_writer.extruder() == nullptr)
        return gcode;

    // wipe (if it's enabled for this extruder and we have a stored wipe path)
    if (EXTRUDER_CONFIG(wipe) && m_wipe.has_path()) {
        gcode += toolchange ? m_writer.retract_for_toolchange(true) : m_writer.retract(true);
        gcode += m_wipe.wipe(*this, toolchange);
    }

    /*  The parent class will decide whether we need to perform an actual retraction
        (the extruder might be already retracted fully or partially). We call these
        methods even if we performed wipe, since this will ensure the entire retraction
        length is honored in case wipe path was too short.  */
    gcode += toolchange ? m_writer.retract_for_toolchange() : m_writer.retract();

    gcode += m_writer.reset_e();
    if (m_writer.extruder()->retract_length() > 0 || m_config.use_firmware_retraction)
        gcode += m_writer.lift();

    return gcode;
}

std::string GCode::set_extruder(unsigned int extruder_id, double print_z)
{
    if (!m_writer.need_toolchange(extruder_id))
        return "";

    // if we are running a single-extruder setup, just set the extruder and return nothing
    if (!m_writer.multiple_extruders) {
        this->placeholder_parser().set("current_extruder", extruder_id);

        std::string gcode;
        // Append the filament start G-code.
        const std::string &start_filament_gcode = m_config.start_filament_gcode.get_at(extruder_id);
        if (! start_filament_gcode.empty()) {
            // Process the start_filament_gcode for the filament.
            DynamicConfig config;
            config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
            config.set_key_value("layer_z",   new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
            config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
            config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
            gcode += this->placeholder_parser_process("start_filament_gcode", start_filament_gcode, extruder_id, &config);
            check_add_eol(gcode);
        }
        gcode += m_writer.toolchange(extruder_id);
        return gcode;
    }

    // prepend retraction on the current extruder
    std::string gcode = this->retract(true);

    // Always reset the extrusion path, even if the tool change retract is set to zero.
    m_wipe.reset_path();

    if (m_writer.extruder() != nullptr) {
        // Process the custom end_filament_gcode.
        unsigned int        old_extruder_id     = m_writer.extruder()->id();
        const std::string  &end_filament_gcode  = m_config.end_filament_gcode.get_at(old_extruder_id);
        if (! end_filament_gcode.empty()) {
            gcode += placeholder_parser_process("end_filament_gcode", end_filament_gcode, old_extruder_id);
            check_add_eol(gcode);
        }
    }


    // If ooze prevention is enabled, set current extruder to the standby temperature.
    if (m_ooze_prevention.enable && m_writer.extruder() != nullptr)
        gcode += m_ooze_prevention.pre_toolchange(*this);

    const std::string& toolchange_gcode = m_config.toolchange_gcode.value;
    std::string toolchange_gcode_parsed;

    // Process the custom toolchange_gcode. If it is empty, insert just a Tn command.
    if (!toolchange_gcode.empty()) {
        DynamicConfig config;
        config.set_key_value("previous_extruder", new ConfigOptionInt((int)(m_writer.extruder() != nullptr ? m_writer.extruder()->id() : -1 )));
        config.set_key_value("next_extruder",     new ConfigOptionInt((int)extruder_id));
        config.set_key_value("layer_num",         new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z",           new ConfigOptionFloat(print_z));
        config.set_key_value("toolchange_z",      new ConfigOptionFloat(print_z));
        config.set_key_value("max_layer_z",       new ConfigOptionFloat(m_max_layer_z));
        toolchange_gcode_parsed = placeholder_parser_process("toolchange_gcode", toolchange_gcode, extruder_id, &config);
        gcode += toolchange_gcode_parsed;
        check_add_eol(gcode);
    }

    // We inform the writer about what is happening, but we may not use the resulting gcode.
    std::string toolchange_command = m_writer.toolchange(extruder_id);
    if (! custom_gcode_changes_tool(toolchange_gcode_parsed, m_writer.toolchange_prefix(), extruder_id))
        gcode += toolchange_command;
    else {
        // user provided his own toolchange gcode, no need to do anything
    }

    // Set the temperature if the wipe tower didn't (not needed for non-single extruder MM)
    if (m_config.single_extruder_multi_material && !m_config.wipe_tower) {
        int temp = (m_layer_index <= 0 ? m_config.first_layer_temperature.get_at(extruder_id) :
                                         m_config.temperature.get_at(extruder_id));

        gcode += m_writer.set_temperature(temp, false);
    }

    this->placeholder_parser().set("current_extruder", extruder_id);

    // Append the filament start G-code.
    const std::string &start_filament_gcode = m_config.start_filament_gcode.get_at(extruder_id);
    if (! start_filament_gcode.empty()) {
        // Process the start_filament_gcode for the new filament.
        DynamicConfig config;
        config.set_key_value("layer_num", new ConfigOptionInt(m_layer_index));
        config.set_key_value("layer_z",   new ConfigOptionFloat(this->writer().get_position().z() - m_config.z_offset.value));
        config.set_key_value("max_layer_z", new ConfigOptionFloat(m_max_layer_z));
        config.set_key_value("filament_extruder_id", new ConfigOptionInt(int(extruder_id)));
        gcode += this->placeholder_parser_process("start_filament_gcode", start_filament_gcode, extruder_id, &config);
        check_add_eol(gcode);
    }
    // Set the new extruder to the operating temperature.
    if (m_ooze_prevention.enable)
        gcode += m_ooze_prevention.post_toolchange(*this);

    return gcode;
}

// convert a model-space scaled point into G-code coordinates
Vec2d GCode::point_to_gcode(const Point &point) const
{
    Vec2d extruder_offset = EXTRUDER_CONFIG(extruder_offset);
    return unscaled<double>(point) + m_origin - extruder_offset;
}

Vec2d GCode::point_to_gcode_quantized(const Point &point) const
{
    Vec2d p = this->point_to_gcode(point);
    return { GCodeFormatter::quantize_xyzf(p.x()), GCodeFormatter::quantize_xyzf(p.y()) };
}

// convert a model-space scaled point into G-code coordinates
Point GCode::gcode_to_point(const Vec2d &point) const
{
    Vec2d pt = point - m_origin;
    if (const Extruder *extruder = m_writer.extruder(); extruder)
        // This function may be called at the very start from toolchange G-code when the extruder is not assigned yet.
        pt += m_config.extruder_offset.get_at(extruder->id());
    return scaled<coord_t>(pt);
        
}

}   // namespace Slic3r
