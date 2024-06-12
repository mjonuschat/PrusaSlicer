#include "Scalable.hpp"
#include "wxExtensions.hpp"
#include "BitmapGetters.hpp"
#include "StringConversions.hpp"
#include "WidgetsConfig.hpp"

#include <wx/display.h>

#include <set>

namespace Slic3r::App::WX {

// ----------------------------------------------------------------------------
// ScalableBitmap
// ----------------------------------------------------------------------------
ScalableBitmap::ScalableBitmap( wxWindow *parent, 
                                const std::string& icon_name,
                                const int  width/* = 16*/,
                                const int  height/* = -1*/,
                                const bool grayscale/* = false*/):
    m_parent(parent), m_icon_name(icon_name),
    m_bmp_width(width), m_bmp_height(height)
{
    m_bmp = *get_bmp_bundle(icon_name, width, height);
    m_bitmap = m_bmp.GetBitmapFor(m_parent);
}

ScalableBitmap::ScalableBitmap( wxWindow*           parent,
                                const std::string&  icon_name,
                                const wxSize        icon_size,
                                const bool          grayscale/* = false*/) :
ScalableBitmap(parent, icon_name, icon_size.x, icon_size.y, grayscale)
{
}

ScalableBitmap::ScalableBitmap(wxWindow* parent, boost::filesystem::path& icon_path, const wxSize icon_size)
    :m_parent(parent), m_bmp_width(icon_size.x), m_bmp_height(icon_size.y)
{
    wxString path = from_u8(icon_path.string());
    wxBitmap bitmap;
    const std::string ext = icon_path.extension().string();

    if (ext == ".png" || ext == ".jpg") {
        bitmap.LoadFile(path, ext == ".png" ? wxBITMAP_TYPE_PNG : wxBITMAP_TYPE_JPEG);

        // check if the bitmap has a square shape

        if (wxSize sz = bitmap.GetSize(); sz.x != sz.y) {
            const int bmp_side = std::min(sz.GetWidth(), sz.GetHeight());

            wxRect rc = sz.GetWidth() > sz.GetHeight() ?
                        wxRect(int(0.5 * (sz.x - sz.y)), 0, bmp_side, bmp_side) :
                        wxRect(0, int(0.5 * (sz.y - sz.x)), bmp_side, bmp_side);

            bitmap = bitmap.GetSubBitmap(rc);
        }

        // set mask for circle shape

        wxBitmapBundle mask_bmps = *get_bmp_bundle("user_mask", bitmap.GetSize().GetWidth());
        wxMask* mask = new wxMask(mask_bmps.GetBitmap(bitmap.GetSize()), *wxBLACK);
        bitmap.SetMask(mask);

        // get allowed scale factors

        std::set<double> scales = { 1.0 };
#ifdef __APPLE__
        scales.emplace(Slic3r::GUI::mac_max_scaling_factor());
#elif _WIN32
        size_t disp_cnt = wxDisplay::GetCount();
        for (size_t disp = 0; disp < disp_cnt; ++disp)
            scales.emplace(wxDisplay(disp).GetScaleFactor());
#endif

        // create bitmaps for bundle

        wxVector<wxBitmap> bmps;
        for (double scale : scales) {
            wxBitmap bmp = bitmap;
            wxBitmap::Rescale(bmp, icon_size * scale);
            bmps.push_back(bmp);
        }
        m_bmp = wxBitmapBundle::FromBitmaps(bmps);
    }
    else if (ext == ".svg") {
        m_bmp = wxBitmapBundle::FromSVGFile(path, icon_size);
    }
}

wxSize ScalableBitmap::GetSize() const 
{ 
    return get_preferred_size(m_bmp, m_parent); 
}

void ScalableBitmap::sys_color_changed()
{
    m_bmp = *get_bmp_bundle(m_icon_name, m_bmp_width, m_bmp_height);
}

// ----------------------------------------------------------------------------
// PrusaButton
// ----------------------------------------------------------------------------

ScalableButton::ScalableButton( wxWindow *          parent,
                                wxWindowID          id,
                                const std::string&  icon_name /*= ""*/,
                                const wxString&     label /* = wxEmptyString*/,
                                const wxSize&       size /* = wxDefaultSize*/,
                                const wxPoint&      pos /* = wxDefaultPosition*/,
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/,
                                int                 width/* = 16*/, 
                                int                 height/* = -1*/) :
    m_parent(parent),
    m_current_icon_name(icon_name),
    m_bmp_width(width),
    m_bmp_height(height),
    m_has_border(!(style & wxNO_BORDER))
{
    Create(parent, id, label, pos, size, style);
    w_config()->UpdateDarkUI(this);

    if (!icon_name.empty()) {
        SetBitmap(*get_bmp_bundle(icon_name, width, height));
        if (!label.empty())
            SetBitmapMargins(int(0.5* em_unit(parent)), 0);
    }

    if (size != wxDefaultSize)
    {
        const int em = em_unit(parent);
        m_width = size.x/em;
        m_height= size.y/em;
    }
}


ScalableButton::ScalableButton( wxWindow *          parent, 
                                wxWindowID          id,
                                const ScalableBitmap&  bitmap,
                                const wxString&     label /*= wxEmptyString*/, 
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/) :
    m_parent(parent),
    m_current_icon_name(bitmap.name()),
    m_bmp_width(bitmap.px_size().x),
    m_bmp_height(bitmap.px_size().y),
    m_has_border(!(style& wxNO_BORDER))
{
    Create(parent, id, label, wxDefaultPosition, wxDefaultSize, style);
    w_config()->UpdateDarkUI(this);

    SetBitmap(bitmap.bmp());
}

void ScalableButton::SetBitmap_(const ScalableBitmap& bmp)
{
    SetBitmap(bmp.bmp());
    m_current_icon_name = bmp.name();
}

bool ScalableButton::SetBitmap_(const std::string& bmp_name)
{
    m_current_icon_name = bmp_name;
    if (m_current_icon_name.empty())
        return false;

    wxBitmapBundle bmp = *get_bmp_bundle(m_current_icon_name, m_bmp_width, m_bmp_height);
    SetBitmap(bmp);
    SetBitmapCurrent(bmp);
    SetBitmapPressed(bmp);
    SetBitmapFocus(bmp);
    SetBitmapDisabled(bmp);
    return true;
}

void ScalableButton::SetBitmapDisabled_(const ScalableBitmap& bmp)
{
    SetBitmapDisabled(bmp.bmp());
    m_disabled_icon_name = bmp.name();
}

int ScalableButton::GetBitmapHeight()
{
#ifdef __APPLE__
    return GetBitmap().GetScaledHeight();
#else
    return GetBitmap().GetHeight();
#endif
}

wxSize ScalableButton::GetBitmapSize()
{
#ifdef __APPLE__
    return wxSize(GetBitmap().GetScaledWidth(), GetBitmap().GetScaledHeight());
#else
    return wxSize(GetBitmap().GetWidth(), GetBitmap().GetHeight());
#endif
}

void ScalableButton::sys_color_changed()
{
    w_config()->UpdateDarkUI(this, m_has_border);
    if (m_current_icon_name.empty())
        return;
    wxBitmapBundle bmp = *get_bmp_bundle(m_current_icon_name, m_bmp_width, m_bmp_height);
    SetBitmap(bmp);
    SetBitmapCurrent(bmp);
    SetBitmapPressed(bmp);
    SetBitmapFocus(bmp);
    if (!m_disabled_icon_name.empty())
        SetBitmapDisabled(*get_bmp_bundle(m_disabled_icon_name, m_bmp_width, m_bmp_height));
    if (!GetLabelText().IsEmpty())
        SetBitmapMargins(int(0.5 * em_unit(m_parent)), 0);
}

}


