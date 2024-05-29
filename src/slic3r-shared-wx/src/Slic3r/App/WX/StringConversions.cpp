#include "StringConversions.hpp"

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

}


