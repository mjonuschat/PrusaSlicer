#include "SplashScreen.hpp"
#include "MainFrame.hpp"

#include <Slic3r/App/WX/BitmapCache.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include <Slic3r/App/WX/I18N.hpp>

#include "libslic3r/libslic3r_version.h"
#include "libslic3r/Utils.hpp"

#include <wx/font.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/display.h>
#include <wx/fontutil.h>

namespace Slic3r::App::Desktop {

// make a bitmap with dark grey banner on the left side
static wxBitmap MakeBitmap(bool is_editor)
{
    wxBitmap bmp = wxBitmap(WX::from_u8(var(is_editor ? "splashscreen.jpg" : "splashscreen-gcodepreview.jpg")), wxBITMAP_TYPE_JPEG);

    if (!bmp.IsOk()) {
        wxBitmapBundle* bmp_bndl = WX::get_bmp_bundle("PrusaSlicer", 400);
        bmp = bmp_bndl->GetBitmap(bmp_bndl->GetPreferredBitmapSizeAtScale(1.0));
        ASSERT(bmp.IsOk());
    }

    // create dark grey background for the splashscreen
    // It will be 5/3 of the weight of the bitmap
    int width  = lround((double) 5 / 3 * bmp.GetWidth());
    int height = bmp.GetHeight();

    wxImage image(width, height);
    unsigned char* imgdata_ = image.GetData();
    for (int i = 0; i < width * height; ++i) {
        *imgdata_++ = 51;
        *imgdata_++ = 51;
        *imgdata_++ = 51;
    }

    wxBitmap new_bmp(image);

    wxMemoryDC memDC;
    memDC.SelectObject(new_bmp);
    memDC.DrawBitmap(bmp, width - bmp.GetWidth(), 0, true);

    return new_bmp;
}

SplashScreen::SplashScreen(bool is_editor, wxPoint pos) :
    wxSplashScreen(
        MakeBitmap(is_editor),
        wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
        4'000,
        nullptr,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
#ifdef __APPLE__
        wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP
#else
        wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR
#endif // !__APPLE__
    ),
    m_is_editor(is_editor)
{
    m_main_bitmap = m_window->GetBitmap();

    // int init_dpi = get_dpi_for_window(this);
    this->SetPosition(pos);
    // The size of the SplashScreen can be hanged after its moving to another display
    // So, update it from a bitmap size
    this->SetClientSize(m_main_bitmap.GetWidth(), m_main_bitmap.GetHeight());
    this->CenterOnScreen();

    // int new_dpi = get_dpi_for_window(this);
    // m_scale         = (float)(new_dpi) / (float)(init_dpi);
    // scale_bitmap(m_main_bitmap, m_scale);

    // init constant texts and scale fonts
    init_constant_text();

    // this font will be used for the action string
    m_action_font = m_constant_text.credits_font.Bold();

    // draw logo and constant info text
    Decorate(m_main_bitmap);
}

void SplashScreen::SetText(const std::string& text)
{
    set_bitmap(m_main_bitmap);
    if (!text.empty()) {
        wxBitmap bitmap(m_main_bitmap);

        wxMemoryDC memDC;
        memDC.SelectObject(bitmap);

        memDC.SetFont(m_action_font);
        memDC.SetTextForeground(wxColour(237, 107, 33));
        memDC.DrawText(WX::from_u8(text), int(m_scale * 60), m_action_line_y_position);

        memDC.SelectObject(wxNullBitmap);
        set_bitmap(bitmap);
#ifdef __WXOSX__
        // without this code splash screen wouldn't be updated under OSX
        wxYield();
#endif
    }
}

void SplashScreen::Decorate(wxBitmap& bmp)
{
    if (!bmp.IsOk())
        return;

    // draw text to the box at the left of the splashscreen.
    // this box will be 2/5 of the weight of the bitmap, and be at the left.
    int width = lround(bmp.GetWidth() * 0.4);

    // load bitmap for logo
    WX::BitmapCache bmp_cache;
    int logo_size = lround(width * 0.25);
    wxBitmap* logo_bmp_ptr = bmp_cache.load_svg(m_is_editor ? "PrusaSlicer" : "PrusaSlicer-gcodeviewer", logo_size, logo_size);
    if (logo_bmp_ptr == nullptr)
        return;

    wxBitmap logo_bmp = *logo_bmp_ptr;

    wxCoord margin = int(m_scale * 20);

    wxRect banner_rect(wxPoint(0, logo_size), wxPoint(width, bmp.GetHeight()));
    banner_rect.Deflate(margin, 2 * margin);

    // use a memory DC to draw directly onto the bitmap
    wxMemoryDC memDc(bmp);

    // draw logo
    memDc.DrawBitmap(logo_bmp, margin, margin, true);

    // draw the (white) labels inside of our black box (at the left of the splashscreen)
    memDc.SetTextForeground(wxColour(255, 255, 255));

    memDc.SetFont(m_constant_text.title_font);
    memDc.DrawLabel(WX::from_u8(m_constant_text.title), banner_rect, wxALIGN_TOP | wxALIGN_LEFT);

    int title_height = memDc.GetTextExtent(WX::from_u8(m_constant_text.title)).GetY();
    banner_rect.SetTop(banner_rect.GetTop() + title_height);
    banner_rect.SetHeight(banner_rect.GetHeight() - title_height);

    memDc.SetFont(m_constant_text.version_font);
    memDc.DrawLabel(WX::from_u8(m_constant_text.version), banner_rect, wxALIGN_TOP | wxALIGN_LEFT);
    int version_height = memDc.GetTextExtent(WX::from_u8(m_constant_text.version)).GetY();

    memDc.SetFont(m_constant_text.credits_font);
    memDc.DrawLabel(WX::from_u8(m_constant_text.credits), banner_rect, wxALIGN_BOTTOM | wxALIGN_LEFT);
    int credits_height = memDc.GetMultiLineTextExtent(WX::from_u8(m_constant_text.credits)).GetY();
    int text_height    = memDc.GetTextExtent(WX::from_u8("text")).GetY();

    // calculate position for the dynamic text
    int logo_and_header_height = margin + logo_size + title_height + version_height;
    m_action_line_y_position = logo_and_header_height + 0.5 * (bmp.GetHeight() - margin - credits_height - logo_and_header_height - text_height);
}

void SplashScreen::ConstantText::init(wxFont init_font, bool is_editor)
{
    // title
    title = SLIC3R_APP_NAME; // is_editor ? SLIC3R_APP_NAME : GCODEVIEWER_APP_NAME;

    // dynamically get the version to display
    version = L("Version") + " " + std::string(SLIC3R_VERSION);

    // credits infornation
    credits = "\n" + title + " " + L("is based on Slic3r by Alessandro Ranellucci and the RepRap community.") + "\n\n" + L("Developed by Prusa Research.") + "\n\n" + L("Licensed under GNU AGPLv3.") + "\n\n\n\n\n\n\n";

    title_font = version_font = credits_font = init_font;
}

void SplashScreen::init_constant_text()
{
    m_constant_text.init(WX::w_config()->normal_font() /*get_default_font(this)*/, m_is_editor);

    // As default we use a system font for current display.
    // Scale fonts in respect to banner width

    int text_banner_width = lround(0.4 * m_main_bitmap.GetWidth()) - roundl(m_scale * 50); // banner_width - margins

    float title_font_scale = (float) text_banner_width
        / GetTextExtent(WX::from_u8(m_constant_text.title)).GetX();
    scale_font(m_constant_text.title_font, title_font_scale > 3.5f ? 3.5f : title_font_scale);

    float version_font_scale = (float) text_banner_width
        / GetTextExtent(WX::from_u8(m_constant_text.version)).GetX();
    scale_font(m_constant_text.version_font, version_font_scale > 2.f ? 2.f : version_font_scale);

    // The width of the credits information string doesn't respect to the banner width some times.
    // So, scale credits_font in the respect to the longest string width
    wxString credits         = WX::from_u8(m_constant_text.credits);
    int longest_string_width = word_wrap_string(credits);
    m_constant_text.credits  = WX::into_u8(credits);
    float font_scale         = (float) text_banner_width / longest_string_width;
    scale_font(m_constant_text.credits_font, font_scale);
}

void SplashScreen::set_bitmap(wxBitmap& bmp)
{
    m_window->SetBitmap(bmp);
    m_window->Refresh();
    m_window->Update();
}

void SplashScreen::scale_bitmap(wxBitmap& bmp, float scale)
{
    if (scale == 1.0)
        return;

    wxImage image = bmp.ConvertToImage();
    if (!image.IsOk() || image.GetWidth() == 0 || image.GetHeight() == 0)
        return;

    int width  = int(scale * image.GetWidth());
    int height = int(scale * image.GetHeight());
    image.Rescale(width, height, wxIMAGE_QUALITY_BILINEAR);

    bmp = wxBitmap(std::move(image));
}

void SplashScreen::scale_font(wxFont& font, float scale)
{
#ifdef __WXMSW__
    // Workaround for the font scaling in respect to the current active display,
    // not for the primary display, as it's implemented in Font.cpp
    // See https://github.com/wxWidgets/wxWidgets/blob/master/src/msw/font.cpp
    // void wxNativeFontInfo::SetFractionalPointSize(float pointSizeNew)
    wxNativeFontInfo nfi = *font.GetNativeFontInfo();
    float pointSizeNew   = wxDisplay(this).GetScaleFactor() * scale * font.GetPointSize();
    // nfi.lf.lfHeight      = nfi.GetLogFontHeightAtPPI(pointSizeNew, get_dpi_for_window(this));
    nfi.lf.lfHeight = nfi.GetLogFontHeightAtPPI(pointSizeNew, this->GetDPIScaleFactor());
    nfi.pointSize   = pointSizeNew;
    font            = wxFont(nfi);
#else
    font.Scale(scale);
#endif //__WXMSW__
}

// wrap a string for the strings no longer then 55 symbols
// return extent of the longest string
int SplashScreen::word_wrap_string(wxString& input)
{
    size_t line_len = 55; // count of symbols in one line
    int idx         = -1;
    size_t cur_len  = 0;

    wxString longest_sub_string;
    auto get_longest_sub_string = [input](wxString& longest_sub_str, size_t cur_len, size_t i)
    {
        if (cur_len > longest_sub_str.Len())
            longest_sub_str = input.SubString(i - cur_len + 1, i);
    };

    for (size_t i = 0; i < input.Len(); i++) {
        cur_len++;
        if (input[i] == ' ')
            idx = i;
        if (input[i] == '\n') {
            get_longest_sub_string(longest_sub_string, cur_len, i);
            idx     = -1;
            cur_len = 0;
        }
        if (cur_len >= line_len && idx >= 0) {
            get_longest_sub_string(longest_sub_string, cur_len, i);
            input[idx] = '\n';
            cur_len    = i - static_cast<size_t>(idx);
        }
    }

    return GetTextExtent(longest_sub_string).GetX();
}

} // namespace Slic3r::App::Desktop
