#include "StringConversions.hpp"

#include <wx/numformatter.h>

#include "libslic3r/LocalesUtils.hpp" //!


namespace Slic3r::App::WX {

wxString from_u8(const std::string& str)
{
    return wxString::FromUTF8(str.c_str());
}

std::string into_u8(const wxString& str)
{
    auto buffer_utf8 = str.utf8_str();
    return std::string(buffer_utf8.data());
}

wxString from_path(const boost::filesystem::path& path)
{
#ifdef _WIN32
    return wxString(path.string<std::wstring>());
#else
    return from_u8(path.string<std::string>());
#endif
}

boost::filesystem::path into_path(const wxString& str)
{
    return boost::filesystem::path(str.wx_str());
}

wxString double_to_string(double const value, const int max_precision /*= 4*/)
{
// Style_NoTrailingZeroes does not work on OSX. It also does not work correctly with some locales on Windows.
// return wxNumberFormatter::ToString(value, max_precision, wxNumberFormatter::Style_NoTrailingZeroes);

    wxString s = wxNumberFormatter::ToString(value, std::abs(value) < 0.0001 ? 10 : max_precision, wxNumberFormatter::Style_None);

    // The following code comes from wxNumberFormatter::RemoveTrailingZeroes(wxString& s)
    // with the exception that here one sets the decimal separator explicitely to dot.
    // If number is in scientific format, trailing zeroes belong to the exponent and cannot be removed.
    if (s.find_first_of("eE") == wxString::npos) {
        char dec_sep = is_decimal_separator_point() ? '.' : ',';
        const size_t posDecSep = s.find(dec_sep);
        // No decimal point => removing trailing zeroes irrelevant for integer number.
        if (posDecSep != wxString::npos) {
            // Find the last character to keep.
            size_t posLastNonZero = s.find_last_not_of("0");
            // If it's the decimal separator itself, don't keep it either.
            if (posLastNonZero == posDecSep)
                -- posLastNonZero;
            s.erase(posLastNonZero + 1);
            // Remove sign from orphaned zero.
            if (s.compare("-0") == 0)
                s = "0";
        }
    }

    return s;
}

}

