#include "PrusaFile.hpp"
#include <string_view>
#include <set>
#include <type_traits> // enable_if
#include <boost/assign.hpp>
#include <boost/bimap.hpp>
#include <boost/filesystem.hpp>
#include "nlohmann/json.hpp"
#include "libslic3r/NSVGUtils.hpp" // open content of svg file

#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"

namespace Slic3r {
    extern std::unique_ptr<const Persist3mfData> g_load_from_3mf;
}

using ModelObject = Slic3r::Domain::ModelObject;
using ModelVolume = Slic3r::Domain::ModelVolume;
using ModelInstance = Slic3r::Domain::ModelInstance;
using Model = Slic3r::Domain::Model;
using ModelVolumeType = Slic3r::Domain::ModelVolumeType;
using ModelVolumePtrs = Slic3r::Domain::ModelVolumePtrs;

using json = nlohmann::ordered_json;
using namespace Slic3r;

using EmbossProjection = Domain::EmbossProjection;
using EmbossShape = Domain::EmbossShape;
using EmbossStyle = Domain::EmbossStyle;
using FontProp = Domain::FontProp;
using TextConfiguration = Domain::TextConfiguration;

namespace{
using NamesType = const std::vector<std::string_view>;
using RT = Read3mfIssueType; // shorting name

void write_file(mz_zip_archive &archive, const json &data, const char *filepath) {
    if (data.empty())
        return; // Do not store empty files

    int indentation = 2; // two spaces indentation
    std::string data_str = Biz::beautify_json(data, indentation);
    if (!mz_zip_writer_add_mem(&archive, filepath, 
        (const void *) data_str.data(), data_str.length(), MZ_DEFAULT_COMPRESSION))
        throw boost::filesystem::filesystem_error( std::string() +
            "Unable to add \"" + filepath + "\" file into archive.", {});
}

void write_svg_file(mz_zip_archive &archive, const std::string &filepath, const std::string &file_data_str) {
    if(!mz_zip_writer_add_mem(&archive, filepath.c_str(), 
        (const void *) file_data_str.c_str(), file_data_str.size(), MZ_DEFAULT_COMPRESSION))
        throw boost::filesystem::filesystem_error("Unable to add svg file to archive.", {});
}

// Add only non empty data
void add(json& result, std::string_view name, json &&data) {
    if (!data.empty())
        result[name] = std::move(data);
}

/// <summary>
/// Hepl function to convert json data type
/// </summary>
/// <typeparam name="T">Type of data handled by json library</typeparam>
/// <param name="data_json">json keeping value data</param>
/// <param name="value">value to convert into</param>
/// <param name="result">keep list of issues</param>
/// <param name="issue">Current issue type to add when issue appear</param>
/// <returns>True when json contain data and it was converted into value otherwise FALSE</returns>
template<typename T, std::enable_if_t<
       std::is_same_v<T, json::array_t>
    || std::is_same_v<T, json::object_t>
    || std::is_same_v<T, json::string_t>
    || std::is_same_v<T, json::boolean_t>
    || std::is_same_v<T, json::number_integer_t>
    || std::is_same_v<T, json::number_unsigned_t>
    || std::is_same_v<T, json::number_float_t> 
    || std::is_same_v<T, json::binary_t> 
    , bool> = true > 
bool get_value(const json &data_json, T &value, ResultLoad3mf &result, RT issue) {
    assert(data_json.is_primitive());
    assert(!data_json.is_structured());
    assert(!data_json.is_null());
    // Get pointer on data stored in json object
    // No-throw guarantee: this function never throws exceptions. (cite from documentation)
    const T *value_ptr = data_json.get_ptr<const T*>();
    if (value_ptr == nullptr) {
        result.add(issue, std::string("Can't get value"), data_json.dump());
        return false;
    }
    value = *value_ptr; // copy value    
    return true;
}

/// <summary>
/// Function to convert values from json with adding result issue when data is not same as saved
/// Prefer call of from_json
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="data_json"></param>
/// <param name="value"></param>
/// <param name="r"></param>
/// <param name="issue"></param>
/// <returns></returns>
template<typename T>
bool value_from_json(const json &data_json, T &value, ResultLoad3mf &r, RT issue) {
    // Do not know the way to template specify only when type alias differ
    // Get an error template function ambiguity
    // NOTE: on R-PI is same type: unsigned = size_t = json::number_unsigned_t
    if constexpr (std::is_floating_point_v<T>){ // float + double
        if (!data_json.is_number()) { r.add(issue, std::string("Not a number"), data_json.dump()); return false; }
        json::number_float_t value_dbl; if (!value_from_json(data_json, value_dbl, r, issue)) return false;
        value = static_cast<T>(value_dbl); return true;
    } else if constexpr (std::is_signed_v<T>) { // int
        if (!data_json.is_number_integer()) { r.add(issue, std::string("Not a number integer"), data_json.dump()); return false; }
        json::number_integer_t value_int64; if (!value_from_json(data_json, value_int64, r, issue)) return false;
        value = static_cast<T>(value_int64); return true; 
    } else if constexpr (std::is_unsigned_v<T>) { // unsigned + size_t
        if (!data_json.is_number_unsigned()) { r.add(issue, std::string("Not an unsigned number"), data_json.dump()); return false; }
        json::number_unsigned_t value_uint64; if (!value_from_json(data_json, value_uint64, r, issue)) return false;
        value = static_cast<T>(value_uint64); return true;
    }
    // template specialization is not implemented
    assert(false);
    return false;
}

template<> bool value_from_json(const json &data_json, json::number_unsigned_t &value, ResultLoad3mf &r, RT issue) {
    if (!data_json.is_number_unsigned()) { r.add(issue, std::string("Not an unsigned number"), data_json.dump()); return false; }
    return get_value(data_json, value, r, issue);}
template<> bool value_from_json(const json &data_json, json::number_integer_t &value, ResultLoad3mf &r, RT issue) {
    if (!data_json.is_number())          { r.add(issue, std::string("Not an integer number"), data_json.dump()); return false; }
    return get_value(data_json, value, r, issue);}
template<> bool value_from_json(const json &data_json, json::string_t &value, ResultLoad3mf &r, RT issue) {
    if (!data_json.is_string())          { r.add(issue, std::string("Not a string"), data_json.dump()); return false; }
    return get_value(data_json, value, r, issue);}
template<> bool value_from_json(const json &data_json, json::boolean_t &value, ResultLoad3mf &r, RT issue) {
    if (!data_json.is_boolean())         { r.add(issue, std::string("Not a bool"), data_json.dump()); return false; }
    return get_value(data_json, value, r, issue);}
template<> bool value_from_json(const json &data_json, json::number_float_t &value, ResultLoad3mf &r, RT issue) {
    if (data_json.is_number_float()) return get_value(data_json, value, r, issue);
    // Load int value into floating point value without issue
    if (!data_json.is_number_integer())  { r.add(issue, std::string("Not a number"), data_json.dump()); return false; }
    json::number_integer_t value_int; if (!value_from_json(data_json, value_int, r, issue)) return false;
    value = static_cast<json::number_float_t>(value_int); return true;}

template<typename T> // Vec3d, Vec3f, Vec3i
json to_json(const Eigen::Matrix<T, 3, 1, Eigen::DontAlign> &v) { return json{v.x(), v.y(), v.z()}; }
template<typename T> // Vec3d, Vec3f, Vec3i
bool value_from_json(const json &v_json, Eigen::Matrix<T, 3, 1, Eigen::DontAlign> &v, ResultLoad3mf& r, RT issue) {
    if (!v_json.is_array()) {
        r.add(issue, std::string("not an array"), v_json.dump());
        return false;
    }
    size_t max_i = 3;
    if (v_json.size() != 3) {
        r.add(issue, std::string("bad amount of numbers"), std::to_string(v_json.size()), v_json.dump());
        max_i = std::min(size_t(3), v_json.size());        
    }
    for (size_t i = 0; i < max_i; i++)
        value_from_json(v_json[i], v[i], r, issue);
    return (max_i == 3);
}

// optional as value
template<typename T> bool value_from_json(const json &data_json, std::optional<T> &value, ResultLoad3mf &r, RT issue){
    T value_; if (!value_from_json(data_json, value_, r, issue)) return false;
    value = value_; return true;}

/// <summary>
/// Get value by name from parent json object 
/// </summary>
/// <typeparam name="T">Specify loaded data</typeparam>
/// <param name="parent_json">MUST be json object(NOTE: check is done in function is_valid())</param>
/// <param name="name">Json object Property name - in JSON file is between symbols "(appostrofs)</param>
/// <param name="value">[output] value to fill by data from json</param>
/// <param name="result">Keep list of issues collected during load</param>|
/// <param name="issue">Specify issue genereted when JSON data do not match T datatype</param>
/// <param name="log_missing">When true than generate issue missing when parent object do not contain name property</param>
/// <returns>TRUE when succesfully read value from json otherwise FALSE</returns>
template<typename T>
inline bool from_json(const json &parent_json, std::string_view name, T& value, ResultLoad3mf& result,
    RT issue, bool log_missing = false){
    auto json_ptr = parent_json.find(name);
    if (json_ptr == parent_json.end()) {
        if (log_missing)
            result.add(issue, std::string("missing"));
        return false;
    }
    return value_from_json(*json_ptr, value, result, issue);
}

// Convert enum to json by bidirectional map
template<typename ENUM>
json to_json(ENUM value, const boost::bimap<ENUM, std::string_view> &bmap){
    const auto &map = bmap.left;
    auto found_item = map.find(value);

    // not found, add key to enum!!
    // To fix it uses first value in map. Should not appear in released version
    assert(found_item != map.end()); 
    if (found_item == map.end())
        found_item = map.begin();

    return json(found_item->second);
}

template<typename ENUM>
bool from_json(const json &parent_json, std::string_view name, ENUM &value, const boost::bimap<ENUM, std::string_view> &bmap,
    ResultLoad3mf &r, RT issue, bool log_missing = false ) {
    auto json_ptr = parent_json.find(name);
    if (json_ptr == parent_json.end()) {
        if (log_missing)
            r.add(issue, std::string("missing"));
        return false;
    }
    const json &data_json = *json_ptr;
    std::string value_str;
    if (!value_from_json(data_json, value_str, r, issue))
        return false;

    const auto &map = bmap.right;
    auto found_item = map.find(value_str);

    // not known string
    if (found_item == map.end()) {
        r.add(issue, std::string("unknown enum value"), value_str);
        return false;
    }

    value = found_item->second;
    return true;
}

template<typename CONFIG_TYPE> 
// CONFIG_TYPE has method set_deserialize 
// (IMRPOVE: there should be interface)
// DynamicConfig + ModelConfigObject
bool load_configuration(
    const json &parent_json,
    std::string_view name,
    CONFIG_TYPE &config,
    ResultLoad3mf& result,
    RT issue_type,
    ConfigSubstitutionContext &config_substitutions // Remove in future - all information should be already in result load 3mf
    , bool log_missing = false
) {
    auto config_json_it = parent_json.find(name);
    if (config_json_it == parent_json.end()){
        if (log_missing)
            result.add(issue_type, std::string("Missing configuration"));
        return false;
    }
    const json &config_json = *config_json_it;
    if (config_json.empty()) {
        result.add(issue_type, std::string("Configuration is empty"));
        return false;
    }
    if (!config_json.is_object()){
        result.add(issue_type, std::string("Not an object"), config_json.dump());
        return false;
    }
    for (const auto &[key, value_json] : config_json.items()) {
        // key ... const std::string &
        if (key.empty()){
            result.add(issue_type, std::string("Skip empty key"));
            continue;
        }
        std::string value;
        if (!value_from_json(value_json, value, result, issue_type))
            continue; // NOTE: In configuration is empty string valid value
        
        try {
            config.set_deserialize(key, value, config_substitutions);
        } catch (UnknownOptionException& e) {
            result.add(issue_type, std::string("UnknownOptionException"), key, value,
                std::string(e.what()));
        }
    }
    return true;
}

/// <summary>
/// Write issue when object contain not known names
/// Check whether json is object
/// </summary>
/// <param name="object_json">json object</param>
/// <param name="known_properties">list of known property names in object</param>
/// <param name="result">List of issues</param>
/// <param name="issue">Type of issue added when unknown property name appear</param>
/// <returns>False when object_json is not an object otherwise True</returns>
bool is_valid(const json &object_json, NamesType &known_properties, 
    ResultLoad3mf &result, RT issue) {
    if (!object_json.is_object()){
        result.add(issue, std::string("Not an object"), object_json.dump());
        return false;
    }
    for (const auto &[key, value] : object_json.items()) {
        if (std::any_of(known_properties.begin(), known_properties.end(),
            [&kk = key](std::string_view k) {return k==kk;}))
            continue;
        result.add(issue, key, value.dump());
    }
    return true;
}

namespace TextConfigurationSerialization {
// Store / load of TextConfiguration
constexpr std::string_view TEXT = "text";
// TextConfiguration::EmbossStyle
constexpr std::string_view STYLE_NAME = "style_name";
constexpr std::string_view FONT_DESCRIPTOR = "font_descriptor";
constexpr std::string_view FONT_DESCRIPTOR_TYPE = "font_descriptor_type";

// TextConfiguration::FontProperty
constexpr std::string_view CHAR_GAP = "char_gap";
constexpr std::string_view LINE_GAP = "line_gap";
constexpr std::string_view LINE_HEIGHT = "line_height";
constexpr std::string_view BOLDNESS = "boldness";
constexpr std::string_view SKEW = "skew";
constexpr std::string_view PER_GLYPH = "per_glyph";
constexpr std::string_view HORIZONTAL_ALIGN = "horizontal";
constexpr std::string_view VERTICAL_ALIGN = "vertical";
constexpr std::string_view COLLECTION_NUMBER = "collection";

constexpr std::string_view FONT_FAMILY = "family";
constexpr std::string_view FONT_FACE_NAME = "face_name";
constexpr std::string_view FONT_STYLE = "style";
constexpr std::string_view FONT_WEIGHT = "weight";

const NamesType NAMES = {{TEXT, STYLE_NAME, FONT_DESCRIPTOR, FONT_DESCRIPTOR_TYPE, 
CHAR_GAP, LINE_GAP, LINE_HEIGHT, BOLDNESS, SKEW, PER_GLYPH, HORIZONTAL_ALIGN, VERTICAL_ALIGN, COLLECTION_NUMBER,
FONT_FAMILY, FONT_FACE_NAME, FONT_STYLE, FONT_WEIGHT}};

using TypeToName = boost::bimap<EmbossStyle::Type, std::string_view>;
const TypeToName type_to_name = 
    boost::assign::list_of<TypeToName::relation>
    (EmbossStyle::Type::file_path, "file_name")
    (EmbossStyle::Type::wx_win_font_descr, "wxFontDescriptor_Windows")
    (EmbossStyle::Type::wx_lin_font_descr, "wxFontDescriptor_Linux")
    (EmbossStyle::Type::wx_mac_font_descr, "wxFontDescriptor_MacOsX");

using HorizontalAlignToName = boost::bimap<FontProp::HorizontalAlign, std::string_view>;
const HorizontalAlignToName horizontal_align_to_name = 
    boost::assign::list_of<HorizontalAlignToName::relation>
    (FontProp::HorizontalAlign::left, "left")
    (FontProp::HorizontalAlign::center, "center")
    (FontProp::HorizontalAlign::right, "right");

using VerticalAlignToName = boost::bimap<FontProp::VerticalAlign, std::string_view>;
const VerticalAlignToName vertical_align_to_name = 
    boost::assign::list_of<VerticalAlignToName::relation>
    (FontProp::VerticalAlign::top, "top")
    (FontProp::VerticalAlign::center, "middle")
    (FontProp::VerticalAlign::bottom, "bottom");

json to_json(const TextConfiguration &tc) {
    json result = json::object();
    result[TEXT] = tc.text;
    // font item
    const EmbossStyle &style = tc.style;
    result[STYLE_NAME] = style.name;
    result[FONT_DESCRIPTOR] = style.path;
    result[FONT_DESCRIPTOR_TYPE] = ::to_json(style.type, type_to_name);

    // font property
    const FontProp &fp = tc.style.prop;
    if (fp.char_gap.has_value()) result[CHAR_GAP] = *fp.char_gap;
    if (fp.line_gap.has_value()) result[LINE_GAP] = *fp.line_gap;

    result[LINE_HEIGHT] = fp.size_in_mm;
    if (fp.boldness.has_value()) result[BOLDNESS] = *fp.boldness;
    if (fp.skew.has_value())     result[SKEW] = *fp.skew;
    if (fp.per_glyph)            result[PER_GLYPH] = 1;
    result[HORIZONTAL_ALIGN] = ::to_json(fp.align.first, horizontal_align_to_name);
    result[VERTICAL_ALIGN] = ::to_json(fp.align.second, vertical_align_to_name);
    if (fp.collection_number.has_value()) result[COLLECTION_NUMBER] = *fp.collection_number;
    // font descriptor
    if (fp.family.has_value())   result[FONT_FAMILY] = *fp.family;
    if (fp.face_name.has_value())result[FONT_FACE_NAME] = *fp.face_name;
    if (fp.style.has_value())    result[FONT_STYLE] = *fp.style;
    if (fp.weight.has_value())   result[FONT_WEIGHT] = *fp.weight;
    return result;
}

void load(const json &tc_json, TextConfiguration &tc, ResultLoad3mf &result) {
    if(!is_valid(tc_json, NAMES, result, RT::project_text_configuration_unknown_property))
        return;
    from_json(tc_json, TEXT, tc.text, result, RT::project_text_configuration_text_issue, true);
    EmbossStyle &style = tc.style;
    from_json(tc_json, STYLE_NAME,           style.name, result, RT::project_text_configuration_style_name_issue, true);
    from_json(tc_json, FONT_DESCRIPTOR,      style.path, result, RT::project_text_configuration_font_descriptor_issue, true);
    from_json(tc_json, FONT_DESCRIPTOR_TYPE, style.type, type_to_name, result, RT::project_text_configuration_font_descriptor_type_issue, true);
    FontProp &fp = style.prop;
    from_json(tc_json, CHAR_GAP,    fp.char_gap  , result, RT::project_text_configuration_char_gap_issue);
    from_json(tc_json, LINE_GAP,    fp.line_gap  , result, RT::project_text_configuration_line_gap_issue);
    from_json(tc_json, LINE_HEIGHT, fp.size_in_mm, result, RT::project_text_configuration_line_height_issue, true);
    from_json(tc_json, BOLDNESS,    fp.boldness  , result, RT::project_text_configuration_boldness_issue);
    from_json(tc_json, SKEW,        fp.skew      , result, RT::project_text_configuration_skew_issue);
    from_json(tc_json, PER_GLYPH,   fp.per_glyph , result, RT::project_text_configuration_per_glyph_issue);
    from_json(tc_json, HORIZONTAL_ALIGN , fp.align.first , horizontal_align_to_name, result, RT::project_text_configuration_horizontal_align_issue, true);
    from_json(tc_json, VERTICAL_ALIGN   , fp.align.second, vertical_align_to_name  , result, RT::project_text_configuration_vertical_align_issue, true);
    from_json(tc_json, COLLECTION_NUMBER, fp.collection_number, result, RT::project_text_configuration_collection_number_issue);
    from_json(tc_json, FONT_FAMILY      , fp.family           , result, RT::project_text_configuration_font_family_issue);
    from_json(tc_json, FONT_FACE_NAME   , fp.face_name        , result, RT::project_text_configuration_font_face_name_issue);
    from_json(tc_json, FONT_STYLE       , fp.style            , result, RT::project_text_configuration_font_face_style_issue);
    from_json(tc_json, FONT_WEIGHT      , fp.weight           , result, RT::project_text_configuration_font_weight_issue);        
}
} // namespace TextConfigurationSerialization

namespace EmbossShapeSerialization {
// Store / load of EmbossShape
constexpr std::string_view SHAPE_SCALE = "scale";
constexpr std::string_view UNHEALED = "unhealed";
constexpr std::string_view SVG_FILE_PATH = "filepath";
constexpr std::string_view SVG_FILE_PATH_IN_3MF = "filepath3mf";

// EmbossProjection
constexpr std::string_view DEPTH = "depth";
constexpr std::string_view USE_SURFACE = "use_surface";

const NamesType NAMES = {{SHAPE_SCALE, UNHEALED, SVG_FILE_PATH, SVG_FILE_PATH_IN_3MF, DEPTH, USE_SURFACE}};

void write_svg_files(mz_zip_archive &archive, const Domain::Model &model) {
    // write only first appear of svg
    std::set<std::string> paths;
    for (const ModelObject *mo_ptr : model.objects) {
        if (mo_ptr == nullptr)
            continue;
        for (const ModelVolume *mv_ptr : mo_ptr->volumes) {
            if (mv_ptr == nullptr)
                continue;
            if (!mv_ptr->emboss_shape.has_value())
                continue;
            const EmbossShape &es = *mv_ptr->emboss_shape;
            if (!es.svg_file.has_value())
                continue;
            const EmbossShape::SvgFile &svg = *es.svg_file;
            std::shared_ptr<std::string> file_data = svg.file_data; // copy pointer
            if (file_data == nullptr && !svg.path.empty()) {
                file_data = read_from_disk(svg.path);
                if (file_data == nullptr)
                    SPDLOG_WARN("Can't load svg file from path: {}", svg.path);
            }
            if (file_data == nullptr)
                continue; // Text configuration do not have svg file

            // This can appear by pressing [ctr]+[c] continued [ctrl]+[v] of loaded svg from 3mf
            if (!paths.insert(svg.path_in_3mf).second)
                continue; // file is already stored in 3mf
            write_svg_file(archive, svg.path_in_3mf, *file_data);
        }
    }
}

json to_json(const EmbossShape &es) {
    json result = json::object();
    // path_in_3mf .. empty mean shape is EmbossedText OR unwanted store .svg file into .3mf
    // (protection of copyRight)
    if (es.svg_file.has_value() && !es.svg_file->path_in_3mf.empty()) {
        const EmbossShape::SvgFile &svg = *es.svg_file;
        if (!svg.path.empty())
            result[SVG_FILE_PATH] = boost::filesystem::path(svg.path).generic_string();
        result[SVG_FILE_PATH_IN_3MF] = boost::filesystem::path(svg.path_in_3mf).generic_string();
        // INFO: svg file content is stored at begining by function write_svg_files
    }

    result[SHAPE_SCALE] = es.scale;
    if (!es.final_shape.is_healed)
        result[UNHEALED] = true;

    // projection
    const EmbossProjection &p = es.projection;
    result[DEPTH] = p.depth;
    if (p.use_surface)
        result[USE_SURFACE] = true;
    return result;
}
void load(const json &es_json, EmbossShape &es, ResultLoad3mf &result) {
    if (!is_valid(es_json, NAMES, result, RT::project_emboss_shape_unknown_property))
        return;

    EmbossShape::SvgFile svg;
    bool is_source = from_json(es_json, SVG_FILE_PATH, svg.path, result, RT::project_emboss_shape_svg_file_path_issue);
    is_source |= from_json(es_json, SVG_FILE_PATH_IN_3MF, svg.path_in_3mf, result, RT::project_emboss_shape_svg_file_path_in_3mf_issue);
    if (is_source)
        es.svg_file = std::move(svg);

    from_json(es_json, SHAPE_SCALE, es.scale, result, RT::project_emboss_shape_scale_issue, true);
    
    if(bool is_unhealed = false;
        from_json(es_json, UNHEALED, is_unhealed, result, RT::project_emboss_shape_is_unhealed_issue)) {
        assert(is_unhealed);
        es.final_shape.is_healed = false;
    }

    EmbossProjection &p = es.projection;
    from_json(es_json, DEPTH, p.depth, result, RT::project_emboss_shape_depth_issue, true);
    from_json(es_json, USE_SURFACE, p.use_surface, result, RT::project_emboss_shape_use_surface_issue);
}
} // namespace EmbossShapeSerialization

namespace SourceSerialization {

constexpr std::string_view FILEPATH = "filepath";
constexpr std::string_view OFFSET = "offset";
constexpr std::string_view IS_FROM_INCH = "isFromInch";
constexpr std::string_view IS_FROM_METERS = "isFromMeters";
constexpr std::string_view OBJECT_INDEX = "objectIdx"; // index into ModelObjects loaded from 3mf
constexpr std::string_view VOLUME_INDEX = "volumeIdx"; // index into ModelVolumes loaded from 3mf

constexpr std::string_view REPAIR = "repair";
constexpr std::string_view EDGE_FIXED = "edgeFixed";
constexpr std::string_view DEGENERATE_FACETS = "degenerateFacets";
constexpr std::string_view FACETS_REMOVED = "facetsRemoved";
constexpr std::string_view FACETS_REVERSED = "facetsReversed";
constexpr std::string_view BACKWARDED_EDGES = "backwardsEdges";

const NamesType SOURCE_NAMES = {{FILEPATH, OFFSET, IS_FROM_INCH, IS_FROM_METERS, OBJECT_INDEX, VOLUME_INDEX, REPAIR}};
const NamesType REPAIR_NAMES = {{EDGE_FIXED, DEGENERATE_FACETS, FACETS_REMOVED, FACETS_REVERSED, BACKWARDED_EDGES}};
json to_json(const Domain::RepairedMeshErrors &rme) {
    json r;
    if (rme.edges_fixed != 0)
        r[EDGE_FIXED] = rme.edges_fixed;
    if (rme.degenerate_facets != 0)
        r[DEGENERATE_FACETS] = rme.degenerate_facets;
    if (rme.facets_removed != 0)
        r[FACETS_REMOVED] = rme.facets_removed;
    if (rme.facets_reversed != 0)
        r[FACETS_REVERSED] = rme.facets_reversed;
    if (rme.backwards_edges != 0)
        r[BACKWARDED_EDGES] = rme.backwards_edges;
    return r;
}

json to_json(const ModelVolume::Source &source, const Domain::TriangleMeshStats &stats) {
    json r;
    if (!source.input_file.empty())
        // unify direction of slashes
        r[FILEPATH] = boost::filesystem::path(source.input_file).generic_string();
    
    if (!source.mesh_offset.isApprox(Vec3d::Zero()))
        r[OFFSET] = ::to_json(source.mesh_offset);

    // Adress into multi source(like 3mf) model
    r[OBJECT_INDEX] = source.object_idx;
    r[VOLUME_INDEX] = source.volume_idx;

    // can't be both but data allowed it.
    // TODO: fix it inside source to one variable model unit
    assert(!(source.is_converted_from_inches && source.is_converted_from_meters));
    if (source.is_converted_from_inches) {
        r[IS_FROM_INCH] = true;
    } else if (source.is_converted_from_meters) {
        r[IS_FROM_METERS] = true;
    }
    add(r, REPAIR, to_json(stats.repaired_errors));
    return r;
}

void load(const json &source_json, ModelVolume::Source &source, Domain::TriangleMeshStats &stats, ResultLoad3mf &result) {
    if (!is_valid(source_json, SOURCE_NAMES, result, RT::project_source_unknown_property))
        return;

    from_json(source_json, FILEPATH,     source.input_file,  result, RT::project_source_filepath_issue);
    from_json(source_json, OFFSET,       source.mesh_offset, result, RT::project_source_offset_issue);
    from_json(source_json, OBJECT_INDEX, source.object_idx,  result, RT::project_source_object_idx_issue);
    from_json(source_json, VOLUME_INDEX, source.volume_idx,  result, RT::project_source_volume_idx_issue);
    if(!from_json(source_json, IS_FROM_INCH,   source.is_converted_from_inches, result, RT::project_source_is_from_inch_issue))
        from_json(source_json, IS_FROM_METERS, source.is_converted_from_inches, result, RT::project_source_is_from_meters_issue);

    if (auto repair_json_it = source_json.find(REPAIR); repair_json_it != source_json.end()) {
        const json &repair_json = *repair_json_it;
        if(!is_valid(repair_json, REPAIR_NAMES, result, RT::project_source_repair_issue))
            return;

        Domain::RepairedMeshErrors &rme = stats.repaired_errors;
        from_json(repair_json, EDGE_FIXED,        rme.edges_fixed,       result, RT::project_repair_edge_fixed_issue);
        from_json(repair_json, DEGENERATE_FACETS, rme.degenerate_facets, result, RT::project_repair_degenerate_facets_issue);
        from_json(repair_json, FACETS_REMOVED,    rme.facets_removed,    result, RT::project_repair_facets_removed_issue);
        from_json(repair_json, FACETS_REVERSED,   rme.facets_reversed,   result, RT::project_repair_facets_reversed_issue);
        from_json(repair_json, BACKWARDED_EDGES,  rme.backwards_edges,   result, RT::project_repair_backwards_edges_issue);
    }
}
} // namespace SourceSerialization

namespace FacetsAnnotationSerialization {
constexpr const char *FACETS_ANNOTATION_FILE = "Metadata/Slic3r_facets_annotation.json";
constexpr std::string_view ID = "id";
constexpr std::string_view MM_SEGMENTATION_FACETS = "mmSegmentationFacets";
constexpr std::string_view SUPPORT_FACETS = "supportedFacets";
constexpr std::string_view SEAM_FACETS = "seamFacets";
NamesType FACETS_NAMES{{ID, MM_SEGMENTATION_FACETS, SUPPORT_FACETS, SEAM_FACETS}};
constexpr std::string_view TRIANGLE = "triangle"; // index into mesh(specifiead by ID) triangles
constexpr std::string_view DIVIDING = "dividing";
NamesType FACET_NAMES{{TRIANGLE, DIVIDING}};
void write(mz_zip_archive &archive, const Domain::Model &model, const VolumeToObjectid &v2id) {
    auto facets_to_json = [](const Domain::FacetsAnnotation &facets, int triangle_count) {
        if (facets.empty())
            return json{};

        json result = json::array();
        // Should be unique in archive
        for (int i = 0; i < triangle_count; ++i) {
            std::string data = facets.get_triangle_as_string(i);
            if (data.empty())
                continue;
            json t;
            t[TRIANGLE] = i;
            t[DIVIDING] = data;
            result.push_back(std::move(t));
        }
        return result;
    };

    json facets_json = json::array();
    for (const ModelObject *mo : model.objects) {
        for (const ModelVolume *mv : mo->volumes) {
            auto it = v2id.find(mv->id().id);
            if (it == v2id.end())
                continue;
            unsigned id = it->second;
            int triangle_count = static_cast<int>(mv->mesh().its.indices.size());
            json facet_json;
            add(facet_json, MM_SEGMENTATION_FACETS, facets_to_json(mv->mm_segmentation_facets, triangle_count));
            add(facet_json, SUPPORT_FACETS, facets_to_json(mv->supported_facets, triangle_count));
            add(facet_json, SEAM_FACETS, facets_to_json(mv->seam_facets, triangle_count));
            if (facet_json.empty())
                continue;
            facet_json[ID] = id;
            facets_json.push_back(std::move(facet_json));
        }
    }
    write_file(archive, facets_json, FACETS_ANNOTATION_FILE);
}

void load(const json &facets_json_arr, const VolumeMap &volume_map, ResultLoad3mf& result) {
    if (!facets_json_arr.is_array()) {
        result.add(RT::facets_must_be_array);
        return;
    }

    auto json_to_facets = [&result](const json &facets_json, Domain::FacetsAnnotation &facets) {
        if (!facets_json.is_array())
            return;
        facets.reserve(static_cast<int>(facets_json.size()));
        for (const auto& facet_json: facets_json){
            if(!is_valid(facet_json, FACET_NAMES, result, RT::facets_unknown_facet_key))
                continue;

            unsigned triangle_index = 0;
            if (!from_json(facet_json, TRIANGLE, triangle_index, result, RT::facets_triangle_id_issue, true))
                continue;
            std::string data;
            if (!from_json(facet_json, DIVIDING, data, result, RT::facets_dividing_data_issue, true))
                continue;

            // TODO: check setting invalid data !!!
            facets.set_triangle_from_string(triangle_index, data);
        }
        facets.shrink_to_fit();
    };

    for (const auto &volume_facets : facets_json_arr) {
        if (!is_valid(volume_facets, FACETS_NAMES, result, RT::facets_unknown_type))
            continue;
        int id = volume_facets.value(ID, -1);
        if (id < 0) {
            result.add(RT::facets_cant_identify_source, volume_facets.dump());
            continue;
        }
        PathId path_id{static_cast<format_3MF::ST_ResourceID>(id)};
        auto it = volume_map.find(path_id);
        if (it == volume_map.end()) {
            result.add(RT::facets_bad_id, std::to_string(id));
            continue;
        }
        const ModelVolumePtrs &volumes = it->second;
        if (volume_facets.contains(MM_SEGMENTATION_FACETS))
            for (ModelVolume *mv : volumes)
                json_to_facets(volume_facets[MM_SEGMENTATION_FACETS], mv->mm_segmentation_facets);
        if (volume_facets.contains(SUPPORT_FACETS))
            for (ModelVolume *mv : volumes)
                json_to_facets(volume_facets[SUPPORT_FACETS], mv->supported_facets);
        if (volume_facets.contains(SEAM_FACETS))
            for (ModelVolume *mv : volumes)
                json_to_facets(volume_facets[SEAM_FACETS], mv->seam_facets);
    }
}
} // namespace FacetsAnnotationSerialization

// copy _3MF_Exporter::_add_layer_height_profile_file_to_archive
namespace LayerHeightProfileSerialization {
json to_json(const std::vector<double> &layer_height_profile) {
    if (layer_height_profile.empty())
        return {}; // Not used for this object

    assert(layer_height_profile.size() >= 4);
    assert(layer_height_profile.size() %2 == 0);
    if (layer_height_profile.size() < 4 ||
        layer_height_profile.size() % 2 != 0)
        return {}; // bad layer height
            
    // layer_height_profile is list of pair<lo, hi>
    // Slic3r::Layer
    //  .. height  .. hi - lo
    //  .. print_z .. hi + object_print_z_min 
    //  .. slice_z .. 0.5 * (lo + hi)
    json lo_hi_pairs = json::array();
    for (size_t i = 0; i < layer_height_profile.size(); i += 2) {
        double lo = layer_height_profile[i];
        double hi = layer_height_profile[i + 1];
        lo_hi_pairs.push_back({lo, hi});
    }
    return lo_hi_pairs;
}

std::vector<double> load(const json &layer_heights_json, ResultLoad3mf& result) {
    if (!layer_heights_json.is_array()) {
        result.add(RT::layer_heights_must_be_array);
        return {};
    }
    std::vector<double> layer_heights;
    layer_heights.reserve(2 * layer_heights_json.size());
    for (const json &lo_hi_pair : layer_heights_json){
        if (!lo_hi_pair.is_array()) {
            result.add(RT::layer_heights_must_be_array_of_pairs, "no array");
            return {};
        }
        if (lo_hi_pair.size() != 2){
            result.add(RT::layer_heights_must_be_array_of_pairs, std::to_string(lo_hi_pair.size()));
            return {};
        }
        layer_heights.push_back(lo_hi_pair[0].get<double>());
        layer_heights.push_back(lo_hi_pair[1].get<double>());
    }
    return layer_heights;
}
} // namespace LayerHeightProfileSerialization

namespace CutSerialization {

using CutConnectorType = Domain::CutConnectorType;
using CutId = Domain::CutId;

constexpr std::string_view CUT_TYPE = "type";
constexpr std::string_view R_TOLERANCE = "rTolerance";
constexpr std::string_view H_TOLERANCE = "hTolerance";
const NamesType NAMES = {{CUT_TYPE, R_TOLERANCE, H_TOLERANCE}};
using CutConnectorTypeToName = boost::bimap<CutConnectorType, std::string_view>;
const CutConnectorTypeToName cut_type_to_name = 
    boost::assign::list_of<CutConnectorTypeToName::relation>
    (CutConnectorType::Plug, "plug")
    (CutConnectorType::Dowel, "dowel")
    (CutConnectorType::Snap, "snap")
    (CutConnectorType::Undef, "undef"); // TODO: why undef is needed?

json cut_to_json(const ModelVolume::CutInfo &cut_info) {
    json cut_json;
    cut_json[CUT_TYPE] = to_json(cut_info.connector_type, cut_type_to_name);
    cut_json[R_TOLERANCE] = cut_info.radius_tolerance;
    cut_json[H_TOLERANCE] = cut_info.height_tolerance;
    return cut_json;
}

void load(const json &cut_info_json, ModelVolume::CutInfo &cut_info, ResultLoad3mf &result) {
    if (!is_valid(cut_info_json, NAMES, result, RT::project_cut_info_unknown_property))
        return;
    from_json(cut_info_json, CUT_TYPE, cut_info.connector_type, cut_type_to_name, result, RT::project_cut_info_type_issue, true);
    from_json(cut_info_json, R_TOLERANCE, cut_info.radius_tolerance, result, RT::project_cut_info_radius_tolerance_issue, true);
    from_json(cut_info_json, H_TOLERANCE, cut_info.height_tolerance, result, RT::project_cut_info_height_tolerance_issue, true);
}

constexpr std::string_view CUT_ID = "cutId";
constexpr std::string_view CHECK_SUM = "checkSum";
constexpr std::string_view CONNECTORS_CNT = "connectorsCnt";
const NamesType NAMES_ID = {{CUT_TYPE, R_TOLERANCE, H_TOLERANCE}};

json cut_to_json(const CutId& cut) {
    if (!cut.valid())
        return {};

    json cut_json;
    cut_json[CUT_ID] = cut.id();
    cut_json[CHECK_SUM] = cut.check_sum();
    cut_json[CONNECTORS_CNT] = cut.connectors_cnt();
    return cut_json;
}

void load(const json &cut_object_json, CutId &cut, ResultLoad3mf &result){
    if (!is_valid(cut_object_json, NAMES_ID, result, RT::project_cut_info_unknown_property))
        return;

    size_t id, checksum, cnt;
    if(!from_json(cut_object_json, CUT_ID        , id      , result, RT::project_cut_object_id_issue, true)) return;
    if(!from_json(cut_object_json, CHECK_SUM     , checksum, result, RT::project_cut_object_checksum_issue, true)) return;
    if(!from_json(cut_object_json, CONNECTORS_CNT, cnt     , result, RT::project_cut_object_connector_count_issue, true)) return;
    cut = CutId(id, checksum, cnt);
}
} // namespace CutSerialization

namespace VolumeSerialization {
constexpr std::string_view ID                 = "id";                // Object_id from 3mf
constexpr std::string_view VOLUME_UUID        = "object_uuid";       // 3mf Object uuid
constexpr std::string_view VOLUME_TYPE        = "type";              // Slic3r specification of volume type
constexpr std::string_view CONFIGURATION      = "configuration";     // volume specific configuration
constexpr std::string_view TEXT_CONFIGURATION = "textConfiguration"; // more in TextConfigurationSerialization
constexpr std::string_view SHAPE              = "shape";             // more in EmbossShapeSerialization
constexpr std::string_view SOURCE             = "source";            // more in SourceSerialization
constexpr std::string_view CUT_INFO           = "cutInfo";           // more in CutSerialization

NamesType VOLUME_NAMES{{ID, VOLUME_UUID, VOLUME_TYPE, CONFIGURATION, SOURCE, 
                        TEXT_CONFIGURATION, SHAPE, CUT_INFO}};

using VolumeTypeToName = boost::bimap<ModelVolumeType, std::string_view>;
const VolumeTypeToName volume_type_to_name = boost::assign::list_of<VolumeTypeToName::relation>
    (ModelVolumeType::MODEL_PART,         "ModelPart") // default value
    (ModelVolumeType::NEGATIVE_VOLUME,    "NegativeVolume")
    (ModelVolumeType::PARAMETER_MODIFIER, "ParameterModifier")
    (ModelVolumeType::SUPPORT_BLOCKER,    "SupportBlocker")
    (ModelVolumeType::SUPPORT_ENFORCER,   "SupportEnforcer");

json volumes_to_json(const ModelVolumePtrs &volumes, const VolumeToObjectid &v2id, const VolumesWithUUID& volumes_uuid) {
    json volumes_json = json::array();
    for (const ModelVolume *volume_ptr : volumes) {
        const ModelVolume &volume = *volume_ptr;
        json volume_json;
        { // write object id for volume
            auto it = v2id.find(volume.id().id);
            // id is created during writing .model file into 3mf
            assert(it != v2id.end());
            if (it == v2id.end())
                continue;
            volume_json[ID] = it->second;
        }
        if (auto volume_uuid_it = find_by_id(volumes_uuid, volume.id().id);
            volume_uuid_it != volumes_uuid.cend())
            volume_json[VOLUME_UUID] = volume_uuid_it->object_uuid;

        volume_json[VOLUME_TYPE] = to_json(volume.type(), volume_type_to_name);        
        if (volume.text_configuration.has_value())
            add(volume_json, TEXT_CONFIGURATION,
                TextConfigurationSerialization::to_json(*volume.text_configuration));
        if (volume.emboss_shape.has_value())
            add(volume_json, SHAPE, EmbossShapeSerialization::to_json(*volume.emboss_shape));
        add(volume_json, SOURCE, SourceSerialization::to_json(volume.source, volume.mesh().stats()));
        add(volume_json, CONFIGURATION, nlohmann::ordered_json(volume.volume_settings));
        if (volume.is_cut_connector())
            add(volume_json, CUT_INFO, CutSerialization::cut_to_json(volume.cut_info));
        if (!volume_json.empty())
            volumes_json.push_back(std::move(volume_json));
    }
    return volumes_json;
}

void load_volume(const json &volume_json, const VolumeMap &volume_map, ResultLoad3mf& result,
    ConfigSubstitutionContext &config_substitutions) {
    if(!is_valid(volume_json, VOLUME_NAMES, result, RT::project_volume_unknown_property))
        return;

    int id = volume_json.value(ID, -1);
    if (id < 0) {
        result.add(RT::project_volume_missing_id, volume_json.dump());
        return;
    }
    PathId path_id{static_cast<format_3MF::ST_ResourceID>(id)};
    auto it = volume_map.find(path_id);
    if (it == volume_map.end()) {
        result.add(RT::project_volume_bad_id, std::to_string(id));
        return;
    }

    ModelVolumePtrs mvs = it->second;
    if (auto volume_type_it = volume_json.find(VOLUME_TYPE);
        volume_type_it != volume_json.end()){
        std::string volume_type_str = static_cast<std::string>(*volume_type_it);
        const auto &m = volume_type_to_name.right;
        auto type_it = m.find(volume_type_str);
        // type must be defined inside map
        assert(type_it != m.end());
        if (type_it == m.end()) {
            // use default value
            result.add(RT::project_volume_unknown_type, volume_type_str);
        } else {
            ModelVolumeType type = type_it->second;
            for (ModelVolume *mv : mvs)
                mv->set_type(type);
        }
    }

    RT issue = RT::project_volume_config_issue;           
    for (ModelVolume *mv : mvs)
        ; // TODO load_configuration(volume_json, CONFIGURATION, mv->config, result, issue, config_substitutions);
       
    if (auto source_json_it = volume_json.find(SOURCE);
        source_json_it != volume_json.end()){
        for (ModelVolume *mv : mvs) {
            Domain::TriangleMeshStats stats;
            SourceSerialization::load(*source_json_it, mv->source, stats, result);
            // Can't set repaired_errors directly so need to re-set the mesh
            auto &its = const_cast<indexed_triangle_set &>(mv->mesh().its);
            mv->set_mesh(Domain::TriangleMesh(std::move(its), std::move(stats)));
        }
    }

    if (auto tc_json_it = volume_json.find(TEXT_CONFIGURATION);
        tc_json_it != volume_json.end()){
        Domain::TextConfiguration tc;
        TextConfigurationSerialization::load(*tc_json_it, tc, result);
        for (ModelVolume *mv : mvs)
            mv->text_configuration = tc;
    }

    if (auto shape_json_it = volume_json.find(SHAPE);
        shape_json_it != volume_json.end()) {
        Domain::EmbossShape es;
        EmbossShapeSerialization::load(*shape_json_it, es, result);
        for (ModelVolume *mv : mvs)
            mv->emboss_shape = es;
    }

    if (auto cut_json_it = volume_json.find(CUT_INFO);
        cut_json_it != volume_json.end()) {
        ModelVolume::CutInfo cut_info;
        CutSerialization::load(*cut_json_it, cut_info, result);
        for (ModelVolume *mv : mvs)
            mv->cut_info = cut_info;
    }
}
} // namespace VolumeSerialization

namespace SlaSupportPointsSerialization {

constexpr std::string_view POSITION = "p";          // position of support point on Object
constexpr std::string_view HEAD_FRONT_RADIUS = "r"; // head front radius
constexpr std::string_view IS_NEW_ISLAND = "island";        // is new island
NamesType NAMES{{POSITION, HEAD_FRONT_RADIUS, IS_NEW_ISLAND}};

json to_json(const Domain::SLA::SupportPoints &points) {
    json r = json::array();
    for (const Domain::SLA::SupportPoint &p : points) {
        json p_json;
        p_json[POSITION] = ::to_json(p.pos);
        p_json[HEAD_FRONT_RADIUS] = p.head_front_radius;
        if (p.is_island())
            p_json[IS_NEW_ISLAND] = true;
        r.push_back(std::move(p_json));
    }
    return r;
}

void load(const json &pts_json, Domain::SLA::SupportPoints &pts, ResultLoad3mf &result) {
    if (!pts_json.is_array()) {
        result.add(RT::project_sla_support_points_must_be_array);
        return;
    }
    pts.reserve(pts_json.size());
    for (const json &pt_json : pts_json) {
        if(!is_valid(pt_json, NAMES, result, RT::project_sla_support_point_unknown_property))
            continue;
        Domain::SLA::SupportPoint pt;
        bool is_island = pt.is_island();
        from_json(pt_json, POSITION,          pt.pos,               result, RT::project_sla_support_point_position_issue, true);
        from_json(pt_json, HEAD_FRONT_RADIUS, pt.head_front_radius, result, RT::project_sla_support_point_radius_issue, true);
        from_json(pt_json, IS_NEW_ISLAND,     is_island,            result, RT::project_sla_support_point_is_new_island_issue);
        if (is_island)
            pt.type = Domain::SLA::SupportPointType::island;
        pts.push_back(pt);
    }
}
} // namespace SlaSupportPointsSerialization

namespace SlaDrainHolesSerialization {
constexpr std::string_view POSITION = "position";
constexpr std::string_view NORMAL = "normal";
constexpr std::string_view RADIUS = "radius";
constexpr std::string_view HEIGHT = "height";
NamesType NAMES{{POSITION, NORMAL, RADIUS, HEIGHT}};
json to_json(const Domain::SLA::DrainHoles &holes) {
    json r = json::array();
    for (const Domain::SLA::DrainHole &h : holes) {
        json h_json;
        h_json[POSITION] = ::to_json(h.pos);
        h_json[NORMAL]   = ::to_json(h.normal);
        h_json[RADIUS]   = h.radius;
        h_json[HEIGHT]   = h.height;
        r.push_back(std::move(h_json));
    }
    return r;
}
void from_json(const json &holes_json, Domain::SLA::DrainHoles &holes, ResultLoad3mf &result) {
    if (!holes_json.is_array()) {
        result.add(RT::project_sla_drain_holes_must_be_array);
        return;
    }
    holes.reserve(holes_json.size());
    for (const json &hole_json : holes_json) {
        if(!is_valid(hole_json, NAMES, result, RT::project_sla_drain_hole_unknown_property))
            continue;
        Domain::SLA::DrainHole hole;
        ::from_json(hole_json, POSITION, hole.pos,    result, RT::project_sla_drain_hole_position_issue, true);
        ::from_json(hole_json, NORMAL,   hole.normal, result, RT::project_sla_drain_hole_normal_issue, true);
        ::from_json(hole_json, RADIUS,   hole.radius, result, RT::project_sla_drain_hole_radius_issue, true);
        ::from_json(hole_json, HEIGHT,   hole.height, result, RT::project_sla_drain_hole_height_issue, true);
        holes.push_back(hole);
    }
}
} // namespace SlaDrainHolesSerialization

namespace RangesSerialization {

constexpr std::string_view Z_RANGE       = "zRange";        // Range of z values [from, to]
constexpr std::string_view CONFIGURATION = "configuration"; // Range configuration
NamesType RANGES_NAMES{{Z_RANGE, CONFIGURATION}};

json ranges_to_json(const Domain::LayerConfigRanges &ranges) {
    if (ranges.empty())
        return json{};

    json result = json::array();
    for (const auto &[range, config] : ranges) {
        assert(range.first < range.second);
        if (range.first <= range.second)
            continue; // from must be smaller than to

        json config_json = nlohmann::ordered_json(config);
        assert(!config_json.empty());
        if (config_json.empty())
            continue;

        json range_json;
        range_json[Z_RANGE] = range; //{range.first, range.second};
        range_json[CONFIGURATION] = config_json;
        result.push_back(std::move(range_json));
    }
    return result;
}

void ranges_from_json(const json &ranges_json, Domain::LayerConfigRanges &ranges,
    ResultLoad3mf& result, ConfigSubstitutionContext &config_substitutions) {
    if (!ranges_json.is_array()) {
        result.add(RT::project_object_ranges_must_be_array);
        return;
    }
    if (ranges_json.empty()){
        result.add(RT::project_object_ranges_must_not_be_empty);
        return;
    }
    
    for (const json& range_json : ranges_json) {
        if(!is_valid(range_json, RANGES_NAMES, result, RT::project_object_range_unknown_property))
            continue;
        const json& z_range_json = range_json[Z_RANGE];
        if (!z_range_json.is_array() || z_range_json.size() != 2 ) {
            result.add(RT::project_object_range_bad_z1, z_range_json.dump());
            continue;
        }
        Domain::LayerHeightRange z_range = range_json.value(Z_RANGE, Domain::LayerHeightRange(-1, -1));
        if (z_range.first > z_range.second || z_range.first < 0) {
            result.add(RT::project_object_range_bad_z2, z_range_json.dump());
            continue;
        }

        // TODO
        //ModelConfig model_config;
        //RT issue = RT::project_object_range_config_issue;
        //if(load_configuration(range_json, CONFIGURATION, model_config, result, issue, config_substitutions, true))
        //    ranges[z_range] = model_config;
    }
}
} // namespace RangesSerialization

namespace InstanceSerialization{
// identify instance --> .3mf/3D/3dmodel.model/model/build/item
// zero started indexed order of <item> tag inside <build>
constexpr std::string_view ORD = "ord";
constexpr std::string_view ITEM_UUID = "item_uuid"; 
constexpr std::string_view PRINTABLE = "printable"; // true is default and it is not written into 3mf
const NamesType NAMES{{ORD, ITEM_UUID, PRINTABLE}};

json instance_to_json(
    const ModelInstance &instance,
    const InstanceToBuildOrder &instances_map,
    const ItemsWithUUID &items_uuid
) {
    // fast skip til the instance keep only printability state
    if (instance.is_printable())
        return {};
    auto it = instances_map.find(instance.id().id);
    assert(it != instances_map.end()); // must be known 
    if (it == instances_map.end())
        return {};

    json instances_json = json::object();
    instances_json[ORD] = it->second;
    if (auto item_uuid_it = find_by_id(items_uuid, instance.id().id);
        item_uuid_it != items_uuid.cend())
        instances_json[ITEM_UUID] = item_uuid_it->item_uuid;
    assert(!instance.is_printable());
    instances_json[PRINTABLE] = false;
    return instances_json;
}

void load_instance(const json &instance_json, const InstanceMap& instances, ResultLoad3mf &result) {
    if (!is_valid(instance_json, NAMES, result, RT::project_instance_unknown_property))
        return;

    size_t ord;
    if (!from_json(instance_json, ORD, ord, result, RT::project_instance_order_issue, true))
        return;

    if (ord >= instances.size()) {
        result.add(RT::project_instance_order_out_of_range_issue, std::to_string(ord));
        return;
    }

    ModelInstance *mi = instances[ord];
    from_json(instance_json, PRINTABLE, mi->printable, result, RT::project_instance_order_issue);
}

json instances_to_json(
    const Domain::ModelInstancePtrs &instances,
    const InstanceToBuildOrder &instances_map,
    const ItemsWithUUID& items_uuid
) {
    json instances_json = json::array();
    for (const ModelInstance *instance_ptr : instances) {
        assert(instance_ptr != nullptr);
        if (instance_ptr == nullptr)
            continue;
        json instance_json = instance_to_json(*instance_ptr, instances_map, items_uuid);
        if (!instance_json.empty())
            instances_json.push_back(std::move(instance_json));
    }
    return instances_json;
}
} // namespace InstanceSerialization

namespace ObjectsSerialization {
constexpr std::string_view ID                 = "id";             // Object_id from 3mf
constexpr std::string_view OBJECT_UUID        = "object_uuid";    // Universally Unique Identifier of object
constexpr std::string_view VOLUMES            = "volumes";        // List of volume settings
constexpr std::string_view INSTANCES          = "instances";      
constexpr std::string_view CONFIGURATION      = "configuration";  // Object specific DynamicConfig 
constexpr std::string_view LAYER_HEIGHT_PROFILE="layerHeightProfile";
constexpr std::string_view RANGES             = "ranges";         // layer ranges configurations
constexpr std::string_view CUT_OBJECT_ID      = "cutId";          // more in CutSerialization
constexpr std::string_view SLA_SUPPORT_POINTS = "slaSupportPoints";
constexpr std::string_view SLA_DRAIN_HOLES    = "slaDrainHoles";

const NamesType OBJECT_NAMES{{ID, OBJECT_UUID, VOLUMES, INSTANCES, RANGES, CUT_OBJECT_ID, CONFIGURATION, 
                              LAYER_HEIGHT_PROFILE, SLA_SUPPORT_POINTS, SLA_DRAIN_HOLES}};

json object_to_json(const ModelObject &object, const StoredStructure &stored_structure, const Persist3mfData &persist) {
    const ObjectToObjectid &o2id = stored_structure.objects;
    auto it = o2id.find(object.id().id);
    assert(it != o2id.end());
    if (it == o2id.end())
        return {};

    json object_json = json::object();
    object_json[ID] = it->second;

    const ObjectsWithUUID &objects_uuid = persist.objects_uuid;    
    if (auto object_uuid_it = find_by_id(objects_uuid, object.id().id);
        object_uuid_it != objects_uuid.cend())
        object_json[OBJECT_UUID] = object_uuid_it->object_uuid;
    add(object_json, VOLUMES, VolumeSerialization::volumes_to_json(object.volumes, stored_structure.volumes, persist.volumes_uuid));
    add(object_json, INSTANCES, InstanceSerialization::instances_to_json(object.instances, stored_structure.instances, persist.items_uuid));
    if (!object.layer_config_ranges.empty())
        add(object_json, RANGES, RangesSerialization::ranges_to_json(object.layer_config_ranges));
    if (object.is_cut())
        add(object_json, CUT_OBJECT_ID, CutSerialization::cut_to_json(object.cut_id));
    if (!object.object_settings.overrides.empty())
        add(object_json, CONFIGURATION, object.object_settings);
    if (const std::vector<double> &layer_height_profile = object.layer_height_profile.get();
        !layer_height_profile.empty())
        add(object_json, LAYER_HEIGHT_PROFILE, LayerHeightProfileSerialization::to_json(layer_height_profile));
    if (!object.sla_support_points.empty())
        add(object_json, SLA_SUPPORT_POINTS, SlaSupportPointsSerialization::to_json(object.sla_support_points));
    if (!object.sla_drain_holes.empty())
        add(object_json, SLA_DRAIN_HOLES, SlaDrainHolesSerialization::to_json(object.sla_drain_holes));
    return object_json;
}

json objects_to_json(const Model &model, const StoredStructure &stored_structure) {

    assert(g_load_from_3mf);
    if (g_load_from_3mf == nullptr)
        return {};
    const Persist3mfData &persist = *g_load_from_3mf;
    json objects_json = json::array();
    for (const ModelObject *object_ptr : model.objects) {
        assert(object_ptr != nullptr);
        if (object_ptr == nullptr)
            continue;
        json object_json = object_to_json(*object_ptr, stored_structure, persist);
        if (!object_json.empty())
            objects_json.push_back(std::move(object_json));
    }
    return objects_json;
}

void load_objects(
    const json &parent_json,
    std::string_view name,
    const ModelMap &model_map,
    ResultLoad3mf& result,
    ConfigSubstitutionContext &config_substitutions) {
    auto object_json_it = parent_json.find(name);
    if (object_json_it == parent_json.end())
        return; // no objects in json

    const json &objects_json = *object_json_it;
    if (!objects_json.is_array())
        result.add(RT::project_objects_must_be_array);

    const BuildMap &object_map = model_map.build;
    for (const json &object_json : objects_json) {
        if(!is_valid(object_json, OBJECT_NAMES, result, RT::project_object_unknown_property))
            continue;

        if(auto volumes_json = object_json.find(VOLUMES);
            volumes_json != object_json.end() &&
            !volumes_json->empty() &&
            volumes_json->is_array())
            for (const json& volume_json: *volumes_json)
                VolumeSerialization::load_volume(volume_json, model_map.volumes, result, config_substitutions);

        if(auto instances_json = object_json.find(INSTANCES);
            instances_json != object_json.end() &&
            !instances_json->empty() &&
            instances_json->is_array())
            for (const json &instance_json : *instances_json)
                InstanceSerialization::load_instance(instance_json, model_map.instances, result);

        int id = object_json.value(ID, -1);
        if (id < 0) {
            result.add(RT::project_object_missing_id, object_json.dump());
            continue;
        }
        PathId path_id{static_cast<format_3MF::ST_ResourceID>(id)};
        auto it = object_map.find(path_id);
        if (it == object_map.end()) {
            result.add(RT::project_object_bad_id, std::to_string(id));
            continue;
        }
        Domain::ModelObjectPtrs mos = it->second;
        assert(!mos.empty());
        if (mos.empty())
            continue;        

        for (ModelObject *mo_ptr: mos)
            ;// TODO load_configuration(object_json, CONFIGURATION, mo_ptr->config, 
             //   result, RT::project_object_configuration_issue, config_substitutions);

        ModelObject &mo = *mos.front(); // mos is not empty it is checked before
        Domain::ModelObjectPtrs mos_(mos.begin() + 1, mos.end()); // without first model object
        if (auto ranges_json = object_json.find(RANGES);
            ranges_json != object_json.end()) {
            RangesSerialization::ranges_from_json(*ranges_json, mo.layer_config_ranges, result, config_substitutions);
            for (auto mo_ : mos_) // copy into other objects
                mo_->layer_config_ranges = mo.layer_config_ranges;
        }
        if (auto layer_height_profile_json = object_json.find(LAYER_HEIGHT_PROFILE);
            layer_height_profile_json != object_json.end()) {
            mo.layer_height_profile.set(LayerHeightProfileSerialization::load(*layer_height_profile_json, result));
            for (auto mo_ : mos_) // copy into other objects
                mo_->layer_height_profile.set(mo.layer_height_profile.get());
        }
        if (auto sla_points_json = object_json.find(SLA_SUPPORT_POINTS);
            sla_points_json != object_json.end()) {
            SlaSupportPointsSerialization::load(*sla_points_json, mo.sla_support_points, result);
            for (auto mo_ : mos_) // copy into other objects
                mo_->sla_support_points = mo.sla_support_points;
        }
        if (auto sla_holes_json = object_json.find(SLA_DRAIN_HOLES);
            sla_holes_json != object_json.end()) {
            SlaDrainHolesSerialization::from_json(*sla_holes_json, mo.sla_drain_holes, result);
            for (auto mo_ : mos_) // copy into other objects
                mo_->sla_drain_holes = mo.sla_drain_holes;
        }
        if (auto cut_object_json = object_json.find(CUT_OBJECT_ID);
            cut_object_json != object_json.end()) {
            CutSerialization::load(*cut_object_json, mo.cut_id, result);
            for (auto mo_ : mos_) // copy into other objects
                mo_->cut_id = mo.cut_id;
        }
    }
}
} // namespace ObjectsSerialization

struct ResultLoadJson : ResultLoad3mf{
    using ResultLoad3mf::ResultLoad3mf; // use child constructors
    // index to file in archive to detect unprocessed files
    int file_index = -1;
    json parsed_json;
};

ResultLoadJson load_json(mz_zip_archive &archive, const char *filename, RT issue_type) {
    int facets_file_index = mz_zip_reader_locate_file(&archive, filename, nullptr, 0);
    if (facets_file_index < 0)
        return {}; // No facets stored in 3mf

    auto create_issue = [&](const std::string &name) -> ResultLoadJson {
        return {issue_type, name, std::string(filename), std::to_string(facets_file_index)};
    };

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&archive, facets_file_index, &stat))
        return create_issue("mz_zip_reader_file_stat");

    if (stat.m_uncomp_size == 0)
        return create_issue("stat.m_uncomp_size == 0");

    size_t uncomp_size = static_cast<size_t>(stat.m_uncomp_size);
    std::unique_ptr<char[]> buffer(new char[uncomp_size+1]);
    if (mz_zip_reader_extract_to_mem(&archive, facets_file_index, buffer.get(), uncomp_size, 0) !=
        MZ_TRUE)
        return create_issue("mz_zip_reader_extract_to_mem");

    // json must be null terminated
    buffer[uncomp_size] = '\0';

    // Exceptions:
    //      Throws parse_error.101 in case of an unexpected token.
    //      Throws parse_error.102 if to_unicode fails or surrogate error.
    //      Throws parse_error.103 if to_unicode fails.
    // Notes: A UTF-8 byte order mark is silently ignored.
    ResultLoadJson result;
    result.file_index = facets_file_index;
    try {
        result.parsed_json = json::parse(buffer.get());
    } catch (const json::parse_error &e) {
        create_issue(e.what()); 
    }
    return result;
}

namespace ProjectFileSerialization {
constexpr const char *PRUSA_PROJECT_FILEPATH = "metadata/Slic3r_project.json";
constexpr std::string_view CONFIGURATION = "configuration"; // DynamicConfig
constexpr std::string_view OBJECTS = "objects";
NamesType PROJECT_NAMES{{CONFIGURATION, OBJECTS}};
constexpr std::string_view CONFIG_CONTAINERS = "config_containers";

void write(
    mz_zip_archive &archive,
    const Model &model,
    const Domain::Project::ConfigContainerList& config_containers,
    const StoredStructure &stored_structure
) {
    json project_json = json::object();
    add(project_json, OBJECTS, ObjectsSerialization::objects_to_json(model, stored_structure));

    std::vector<nlohmann::json> all_containers_json;
    for (const auto& config_container : config_containers) {
        nlohmann::json cc_json;
        std::vector<nlohmann::json> beds_json;
        for (const auto& bed_instance : config_container->bed_instances()) {
            const Vec3d& offset = bed_instance->transformation.get_offset();
            beds_json.emplace_back();
            beds_json.back()["position_x"] = offset.x();
            beds_json.back()["position_y"] = offset.y();

            const auto& wt = bed_instance->wipe_tower;
            if (wt) {
                beds_json.back()["wipe_tower"]["x"] = wt->position.x();
                beds_json.back()["wipe_tower"]["y"] = wt->position.y();
                beds_json.back()["wipe_tower"]["rotation_angle"] = wt->rotation;
            } else
                beds_json.back()["wipe_tower"] = nullptr;            
        }
        cc_json["beds"] = beds_json;

        const auto& cfg_var = config_container->new_config();
        if (std::holds_alternative<Domain::ConfigPackFDM>(cfg_var))
            cc_json[CONFIGURATION] = nlohmann::ordered_json(Domain::as_boxes(std::get<Domain::ConfigPackFDM>(cfg_var)));
        else if (std::holds_alternative<Domain::ConfigPackSLA>(cfg_var))
            cc_json[CONFIGURATION] = nlohmann::ordered_json(Domain::as_boxes(std::get<Domain::ConfigPackSLA>(cfg_var)));
        else
            PANIC();
        all_containers_json.emplace_back(cc_json);
    }
    project_json[CONFIG_CONTAINERS] = all_containers_json;

    if (project_json.empty())
        return;

    write_file(archive, project_json, PRUSA_PROJECT_FILEPATH);
}

void load(
    const json &project_json,
    const ModelMap &model_map,
    DynamicPrintConfig &config,
    ConfigSubstitutionContext &config_substitutions,
    ResultLoad3mf &result
) {    
    if (!is_valid(project_json, PROJECT_NAMES, result, RT::project_unknown_type))
        return;

    ObjectsSerialization::load_objects(project_json, OBJECTS, model_map, result, config_substitutions);
    load_configuration(project_json, CONFIGURATION, config, result, RT::project_config_issue, config_substitutions);
}
} // namespace ProjectFileSerialization
} // namespace

void Slic3r::store_prusa_files(
    mz_zip_archive &archive,
    const Model &model,
    const Domain::Project::ConfigContainerList& config_containers,
    const StoredStructure &stored_structure
) {
    FacetsAnnotationSerialization::write(archive, model, stored_structure.volumes);
    EmbossShapeSerialization::write_svg_files(archive, model);
    ProjectFileSerialization::write(archive, model, config_containers, stored_structure);
}

PrusaFilesResult Slic3r::load_prusa_files(
    mz_zip_archive &archive,
    const ModelMap &model_map,
    DynamicPrintConfig &config,
    ConfigSubstitutionContext &config_substitutions
) {
    PrusaFilesResult result;
    result.used_file_indices = std::vector<bool>(mz_zip_reader_get_num_files(&archive), {false});

    auto get_json = [&archive, &result](const char *filename, RT err_type)->std::optional<json> {
        ResultLoadJson result_json = load_json(archive, filename, err_type);
        result += static_cast<ResultLoad3mf &>(result_json); // collect issues
        if (result_json.file_index < 0)
            return {};
        result.used_file_indices[result_json.file_index] = true;
        if (result_json.parsed_json.empty())
            return {};
        return result_json.parsed_json;
    };

    if (std::optional<json> project_json = get_json(ProjectFileSerialization::PRUSA_PROJECT_FILEPATH, RT::project_file_is_corrupted);
        project_json.has_value()) 
        ProjectFileSerialization::load(*project_json, model_map, config, config_substitutions, result);

    if (std::optional<json> facets_json = get_json(FacetsAnnotationSerialization::FACETS_ANNOTATION_FILE, RT::facets_annotation_file_is_corrupted);
        facets_json.has_value()) 
        FacetsAnnotationSerialization::load(*facets_json, model_map.volumes, result);

    return result;
}

bool Slic3r::process_embossed_svg(
    mz_zip_archive &archive, const mz_zip_archive_file_stat &stat, 
    Slic3r::Domain::Model &model, ResultLoad3mf& result) 
{
    std::shared_ptr<std::string> data = nullptr;
    std::string filename(stat.m_filename);

    for (const ModelObject *object : model.objects)
    for (ModelVolume *volume : object->volumes) {
        std::optional<EmbossShape> &es = volume->emboss_shape;
        if (!es.has_value())
            continue;
        std::optional<EmbossShape::SvgFile> &svg = es->svg_file;
        if (!svg.has_value())
            continue;

        if (filename.compare(svg->path_in_3mf) != 0)
            continue;

        if (data == nullptr) { // first useage --> load data
            auto file = std::make_unique<std::string>(stat.m_uncomp_size, '\0');
            mz_bool res  = mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, (void *) file->data(), stat.m_uncomp_size, 0);
            if (res == 0) {
                result.add(RT::cant_load_zip_file, filename);
                // it is used svg file but can't be readed from archive.
                return true; 
            }
            data = std::move(file);
        }
        svg->file_data = data; // copy shared pointer
    }

    if (data == nullptr) {
        // SVG is not used as embossed shape
        return false;
    }
    return true;
}
