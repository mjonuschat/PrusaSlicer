#include "StringConversions.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

namespace Slic3r::App::WX {

wxString _(const std::string& s)
{ 
    return Slic3r::App::WX::from_u8(_u8(s)); 
}

wxString _(const wxString& s)
{ 
    return _(Slic3r::App::WX::into_u8(s)); 
}

wxString _L(const std::string& s)
{ 
    return _(std::string(s)); 
}

wxString _CTX(const std::string& s, const std::string& ctx)
{
    return Slic3r::App::WX::from_u8(_CTX_utf8(s, ctx));
}

wxString _L_PLURAL(const std::string& single, const std::string& plural, int n)
{
    return Slic3r::App::WX::from_u8(_L_PLURAL_u8(single, plural, n));
}

}

