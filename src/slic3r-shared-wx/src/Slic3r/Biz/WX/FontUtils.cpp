///|/ Copyright (c) Prusa Research 2021 - 2022 Filip Sykala @Jony01, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/WX/FontUtils.hpp"

#include <boost/log/trivial.hpp>
#include <wx/string.h>
#include <optional>
#include <vector>

//#include "libslic3r/Utils.hpp" // IWYU pragma: keep
//#include "libslic3r/Exception.hpp"

#if defined(__APPLE__)
#include "libslic3r/Utils.hpp" // ScopeGuard
#include <CoreText/CTFont.h>
#include <wx/uri.h>
#include <wx/fontutil.h> // wxNativeFontInfo
#include <wx/osx/core/cfdictionary.h>
#elif defined(__linux__)
#include "Slic3r/Biz/WX/FontConfigHelp.hpp"
#endif

using namespace Slic3r;

#ifdef __APPLE__
namespace {
bool is_valid_ttf(std::string_view file_path)
{
    if (file_path.empty()) return false;
    auto const pos_point = file_path.find_last_of('.');
    if (pos_point == std::string_view::npos) return false;

    // use point only after last directory delimiter
    auto const pos_directory_delimiter = file_path.find_last_of("/\\");
    if (pos_directory_delimiter != std::string_view::npos &&
        pos_point < pos_directory_delimiter)
        return false; // point is before directory delimiter

    // check count of extension chars
    size_t extension_size = file_path.size() - pos_point;
    if (extension_size >= 5) return false; // a lot of symbols for extension
    if (extension_size <= 1) return false; // few letters for extension

    std::string_view extension = file_path.substr(pos_point + 1, extension_size);

    // Because of MacOs - Courier, Geneva, Monaco
    if (extension == std::string_view("dfont")) return false;

    return true;
}

// get filepath from wxFont on Mac OsX
std::string get_file_path(const wxFont& font) {
    const wxNativeFontInfo *info = font.GetNativeFontInfo();
    if (info == nullptr) return {};
    CTFontDescriptorRef descriptor = info->GetCTFontDescriptor();
    CFURLRef typeref = (CFURLRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);
    if (typeref == NULL) return {};
    ScopeGuard sg([&typeref]() { CFRelease(typeref); });
    CFStringRef url = CFURLGetString(typeref);
    if (url == NULL) return {};
    wxString file_uri(wxCFStringRef::AsString(url));
    wxURI uri(file_uri);
    const wxString &path = uri.GetPath();
    wxString path_unescaped = wxURI::Unescape(path);
    std::string path_str = path_unescaped.ToUTF8().data();
    BOOST_LOG_TRIVIAL(trace) << "input uri(" << file_uri.c_str() << ") convert to path(" << path.c_str() << ") string(" << path_str << ").";
    return path_str;
}    
} // namespace
#endif // __APPLE__

namespace{    
wxFont create_wx_font(const wxString& name, const wxFontEncoding encoding)
{ return wxFont(wxFontInfo().FaceName(name).Encoding(encoding)); }
bool is_valid_font(const wxString& name, const std::vector<wxString>& bad, const wxFontEncoding encoding, wxFont& out_wx_font) {
    if (name.empty()) return false;

    // vertical font start with @, we will filter it out
    // Not sure if it is only in Windows so filtering is on all platforms
    if (name[0] == '@') return false;

    // previously detected bad font
    auto it = std::lower_bound(bad.begin(), bad.end(), name);
    if (it != bad.end() && *it == name) return false;

    out_wx_font = create_wx_font(name, encoding);
    //*
    // Faster chech if wx_font is loadable but not 100%
    // names could contain not loadable font
    if (!Slic3r::Biz::WX::can_load(out_wx_font)) return false;

    /*/
    // Slow copy of font files to try load font
    // After this all files are loadable
    auto font_file = Slic3r::Biz::WX::create_font_file(out_wx_font);
    if (font_file == nullptr)
        return false; // can't create font file
    // */
    return true;
}
}

namespace Slic3r::Biz::WX {

bool can_load(const wxFont& font)
{
    if (!font.IsOk()) return false;
#ifdef _WIN32
    return Emboss::can_load(font.GetHFONT()) != nullptr;
#elif defined(__APPLE__)
    return true;
    //return is_valid_ttf(get_file_path(font));
#elif defined(__linux__)
    return true;
    // font config check file path take about 4000ms for chech them all
    //std::string font_path = get_font_path(font);
    //return !font_path.empty();
#endif
    return false;
}

std::vector<wxString> validate_fonts(wxArrayString& facenames, Domain::FontList& valid, std::vector<wxString>& bad, const wxFontEncoding encoding) {
    // NOTE: recreate list of unopenable fonts (bad)
    // 1. filter out nonlisted bad fonts
    // 2. append new founded
    // 3. keep previously founded bad fonts which are still in list
    std::vector<wxString> bad_;
    bad_.reserve(bad.size() + 1); // one more for new installed font
    std::vector<wxString> good;
    good.reserve(facenames.size());
    valid.reserve(facenames.size());

    std::sort(facenames.begin(), facenames.end());
    for (const wxString& name : facenames) {
        wxFont wx_font;
        if (!is_valid_font(name, bad, encoding, wx_font)) {
            bad_.push_back(name);
            continue;
        }
        good.push_back(name);
        valid.push_back(Biz::WX::create_descriptor(wx_font));
    }
    assert(std::is_sorted(bad_.begin(), bad_.end()));
    bad = bad_;
    return good;
}

std::unique_ptr<Domain::FontFile> create_font_file(const wxFont& font)
{
#ifdef _WIN32
    return Emboss::create_font_file(font.GetHFONT());
#elif defined(__APPLE__)
    std::string file_path = get_file_path(font);
    if (!is_valid_ttf(file_path)) {
        BOOST_LOG_TRIVIAL(error) << "Can not process font('" << get_human_readable_name(font) << "'), "
            << "file in path('" << file_path << "') is not valid TTF.";
        return nullptr;
    }
    return Emboss::create_font_file(file_path.c_str());
#elif defined(__linux__)
    std::string font_path = get_font_path(font);
    if (font_path.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Can not read font('" << get_human_readable_name(font) << "'), "
            << "file path is empty.";
        return nullptr;
    }
    return Emboss::create_font_file(font_path.c_str());
#else
    // HERE is place to add implementation for another platform
    // to convert wxFont to font data as windows or font file path as linux
    return nullptr;
#endif
}

Domain::FontDescriptor::Type get_current_type()
{
#ifdef _WIN32
    return Domain::FontDescriptor::Type::wx_win_font_descr;
#elif defined(__APPLE__)
    return Domain::FontDescriptor::Type::wx_mac_font_descr;
#elif defined(__linux__)
    return Domain::FontDescriptor::Type::wx_lin_font_descr;
#else
    return Domain::FontDescriptor::Type::undefined;
#endif
}

Domain::FontDescriptor create_descriptor(const wxFont& font) {
    return Domain::FontDescriptor {
            .name = get_human_readable_name(font),
            .path = store_wxFont(font),
            .type = get_current_type()
    };
}

Domain::EmbossStyle create_emboss_style(const wxFont& font, const std::string& name)
{
    std::string name_item = name.empty() ? get_human_readable_name(font) : name;
    std::string fontDesc = store_wxFont(font);
    Domain::FontDescriptor::Type type = get_current_type();

    // synchronize font property with actual font
    Domain::FontProp font_prop;

    // The point size is defined as 1/72 of the Anglo-Saxon inch (25.4 mm): it
    // is approximately 0.0139 inch or 352.8 um. But it is too small, so I
    // decide use point size as mm for emboss
    font_prop.size_in_mm = font.GetPointSize(); // *0.3528f;

    update_property(font_prop, font);
    return { name_item, fontDesc, type, font_prop };
}

// NOT working on linux GTK2
// load font used by Operating system as default GUI
//EmbossStyle get_os_font()
//{
//    wxSystemFont system_font = wxSYS_DEFAULT_GUI_FONT;
//    wxFont       font        = wxSystemSettings::GetFont(system_font);
//    EmbossStyle  es          = create_emboss_style(font);
//    es.name += std::string(" (OS default)");
//    return es;
//}

std::string get_human_readable_name(const wxFont& font)
{
    if (!font.IsOk()) return "Font is NOT ok.";
    // Face name is optional in wxFont
    wxString name = (!font.GetFaceName().empty()) ?
        font.GetFaceName() :
        (font.GetFamilyString() + wxString::FromUTF8(" ") +
            font.GetStyleString() + wxString::FromUTF8(" ") +
            font.GetWeightString());
    return std::string(name.ToUTF8().data());
}

std::string store_wxFont(const wxFont& font)
{
    // wxString os = wxPlatformInfo::Get().GetOperatingSystemIdName();
    wxString font_descriptor = font.GetNativeFontInfoDesc();
    BOOST_LOG_TRIVIAL(trace) << "'" << font_descriptor << "' wx string get from GetNativeFontInfoDesc. wxFont " <<
        "IsOk(" << font.IsOk() << "), " <<
        "isNull(" << font.IsNull() << ")" <<
        // "IsFree(" << font.IsFree() << "), " << // on MacOs is no function is free
        "IsFixedWidth(" << font.IsFixedWidth() << "), " <<
        "IsUsingSizeInPixels(" << font.IsUsingSizeInPixels() << "), " <<
        "Encoding(" << (int)font.GetEncoding() << "), ";
    return std::string(font_descriptor.ToUTF8().data());
}

wxFont load_wxFont(const std::string& font_descriptor)
{
    BOOST_LOG_TRIVIAL(trace) << "'" << font_descriptor << "'font descriptor string param of load_wxFont()";
    wxString font_descriptor_wx = wxString::FromUTF8(font_descriptor);
    BOOST_LOG_TRIVIAL(trace) << "'" << font_descriptor_wx.ToUTF8().data() << "' wx string descriptor";
    wxFont wx_font(font_descriptor_wx);
    BOOST_LOG_TRIVIAL(trace) << "loaded font is '" << get_human_readable_name(wx_font) << "'.";
    return wx_font;
}

wxFont create_wxFont(const Domain::EmbossStyle& style)
{
    const Domain::FontProp& fp = style.prop;
    double point_size = static_cast<double>(fp.size_in_mm);
    wxFontInfo info(point_size);
    if (fp.family.has_value()) {
        auto it = type_to_family.right.find(*fp.family);
        if (it != type_to_family.right.end()) info.Family(it->second);
    }
    // Face names are not portable, so prefer to use Family() in portable code.
    /* if (fp.face_name.has_value()) {
        wxString face_name(*fp.face_name);
        info.FaceName(face_name);
    }*/
    if (fp.style.has_value()) {
        auto it = type_to_style.right.find(*fp.style);
        if (it != type_to_style.right.end()) info.Style(it->second);
    }
    if (fp.weight.has_value()) {
        auto it = type_to_weight.right.find(*fp.weight);
        if (it != type_to_weight.right.end()) info.Weight(it->second);
    }

    // Improve: load descriptor instead of store to font property to 3mf
    // switch (es.type) {
    // case Domain::FontDescriptor::Type::wx_lin_font_descr:
    // case Domain::FontDescriptor::Type::wx_win_font_descr:
    // case Domain::FontDescriptor::Type::wx_mac_font_descr:
    // case Domain::FontDescriptor::Type::file_path:
    // case Domain::FontDescriptor::Type::undefined:
    // default:
    //}

    wxFont wx_font(info);
    // Check if exist font file
    std::unique_ptr<Domain::FontFile> ff = create_font_file(wx_font);
    if (ff == nullptr) return {};

    return wx_font;
}

void update_property(Domain::FontProp& font_prop, const wxFont& font)
{
    wxString wx_face_name = font.GetFaceName();
    std::string face_name((const char*)wx_face_name.ToUTF8());
    if (!face_name.empty()) font_prop.face_name = face_name;

    wxFontFamily wx_family = font.GetFamily();
    if (wx_family != wxFONTFAMILY_DEFAULT) {
        auto it = type_to_family.left.find(wx_family);
        if (it != type_to_family.left.end()) font_prop.family = it->second;
    }

    wxFontStyle wx_style = font.GetStyle();
    if (wx_style != wxFONTSTYLE_NORMAL) {
        auto it = type_to_style.left.find(wx_style);
        if (it != type_to_style.left.end()) font_prop.style = it->second;
    }

    wxFontWeight wx_weight = font.GetWeight();
    if (wx_weight != wxFONTWEIGHT_NORMAL) {
        auto it = type_to_weight.left.find(wx_weight);
        if (it != type_to_weight.left.end()) font_prop.weight = it->second;
    }
}

bool is_italic(const wxFont& font) {
    wxFontStyle wx_style = font.GetStyle();
    return wx_style == wxFONTSTYLE_ITALIC ||
        wx_style == wxFONTSTYLE_SLANT;
}

bool is_bold(const wxFont& font) {
    wxFontWeight wx_weight = font.GetWeight();
    return wx_weight != wxFONTWEIGHT_NORMAL;
}

std::unique_ptr<Domain::FontFile> set_italic(wxFont& font, const Domain::FontFile& font_file)
{
    static std::vector<wxFontStyle> italic_styles = {
        wxFontStyle::wxFONTSTYLE_ITALIC,
        wxFontStyle::wxFONTSTYLE_SLANT
    };
    wxFontStyle orig_style = font.GetStyle();
    for (wxFontStyle style : italic_styles) {
        font.SetStyle(style);
        std::unique_ptr<Domain::FontFile> new_font_file =
            create_font_file(font);

        // can create italic font?
        if (new_font_file == nullptr) continue;

        // is still same font file pointer?
        if (font_file == *new_font_file) continue;

        return new_font_file;
    }
    // There is NO italic font by wx
    font.SetStyle(orig_style);
    return nullptr;
}

std::unique_ptr<Domain::FontFile> set_bold(wxFont& font, const Domain::FontFile& font_file)
{
    static std::vector<wxFontWeight> bold_weight = {
        wxFontWeight::wxFONTWEIGHT_BOLD,
        wxFontWeight::wxFONTWEIGHT_HEAVY,
        wxFontWeight::wxFONTWEIGHT_EXTRABOLD,
        wxFontWeight::wxFONTWEIGHT_EXTRAHEAVY
    };
    wxFontWeight orig_weight = font.GetWeight();
    for (wxFontWeight weight : bold_weight) {
        font.SetWeight(weight);
        std::unique_ptr<Domain::FontFile> new_font_file =
            create_font_file(font);

        // can create bold font file?
        if (new_font_file == nullptr) continue;

        // is still same font file pointer?
        if (font_file == *new_font_file) continue;

        return new_font_file;
    }
    // There is NO bold font by wx
    font.SetWeight(orig_weight);
    return nullptr;
}
} // namespace Slic3r::Biz::WX
