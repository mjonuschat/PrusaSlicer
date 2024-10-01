///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Hejl @hejllukas, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <wx/button.h>
#include <wx/bmpbndl.h>
#include <boost/filesystem.hpp>

namespace Slic3r::App::WX {

// ----------------------------------------------------------------------------
// ScalableBitmap
// ----------------------------------------------------------------------------

class ScalableBitmap
{
public:
    ScalableBitmap() {};
    ScalableBitmap( wxWindow *parent,
                    const std::string& icon_name,
                    const int  width = 16,
                    const int  height = -1 ,
                    const bool grayscale = false);

    ScalableBitmap( wxWindow *parent,
                    const std::string& icon_name,
                    const  wxSize icon_size,
                    const bool grayscale = false);

    ScalableBitmap( wxWindow                    *parent,
                    boost::filesystem::path&    icon_path,
                    const  wxSize               icon_size);

    ~ScalableBitmap() {}

    void                sys_color_changed();

    const wxBitmapBundle& bmp()   const { return m_bmp; }
    wxBitmap            get_bitmap()    { return m_bmp.GetBitmapFor(m_parent); }
    wxWindow*           parent()  const { return m_parent;}
    const std::string&  name()    const { return m_icon_name; }
    wxSize              px_size()  const { return wxSize(m_bmp_width, m_bmp_height);}

    void                SetBitmap(const wxBitmapBundle& bmp) { m_bmp = bmp; }
    int                 GetWidth()  const { return GetSize().GetWidth(); }
    int                 GetHeight() const { return GetSize().GetHeight(); }
    bool                IsOk()      const { return m_bmp.IsOk(); }
    wxSize              GetSize()   const;

private:
    wxWindow*       m_parent{ nullptr };
    wxBitmapBundle  m_bmp = wxBitmapBundle();
    wxBitmap        m_bitmap = wxBitmap();
    std::string     m_icon_name = "";
    int             m_bmp_width{ 16 };
    int             m_bmp_height{ -1 };
};

// ----------------------------------------------------------------------------
// ScalableButton
// ----------------------------------------------------------------------------

class ScalableButton : public wxButton
{
public:
    ScalableButton(){}
    ScalableButton(
        wxWindow *          parent,
        wxWindowID          id,
        const std::string&  icon_name = "",
        const wxString&     label = wxEmptyString,
        const wxSize&       size = wxDefaultSize,
        const wxPoint&      pos = wxDefaultPosition,
        long                style = wxBU_EXACTFIT | wxNO_BORDER,
        int                 width = 16, 
        int                 height = -1);

    ScalableButton(
        wxWindow *          parent,
        wxWindowID          id,
        const ScalableBitmap&  bitmap,
        const wxString&     label = wxEmptyString,
        long                style = wxBU_EXACTFIT | wxNO_BORDER);

    ~ScalableButton() {}

    void SetBitmap_(const ScalableBitmap& bmp);
    bool SetBitmap_(const std::string& bmp_name);
    void SetBitmapDisabled_(const ScalableBitmap &bmp);
    int  GetBitmapHeight();
    wxSize  GetBitmapSize();

    virtual void    sys_color_changed();

private:
    wxWindow*       m_parent { nullptr };
    std::string     m_current_icon_name;
    std::string     m_disabled_icon_name;
    int             m_width {-1}; // should be multiplied to em_unit
    int             m_height{-1}; // should be multiplied to em_unit

protected:
    // bitmap dimensions 
    int             m_bmp_width{ 16 };
    int             m_bmp_height{ -1 };
    bool            m_has_border {false};
};

}

