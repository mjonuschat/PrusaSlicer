///|/ Copyright (c) Prusa Research 2018 - 2026 Oleksandra Iushchenko @YuSanka, Lukáš Hejl @hejllukas, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <boost/optional.hpp>
#include <wx/wx.h>

class wxTopLevelWindow;

namespace Slic3r::App::WX {

void on_window_geometry(wxTopLevelWindow* tlw, std::function<void()> callback);

class WindowMetrics
{
private:
    wxRect rect;
    bool maximized;

    WindowMetrics() : maximized(false) {}

public:
    static WindowMetrics from_window(wxTopLevelWindow* window);
    static boost::optional<WindowMetrics> deserialize(const std::string& str);

    const wxRect& get_rect() const
    {
        return rect;
    }

    bool get_maximized() const
    {
        return maximized;
    }

    void sanitize_for_display(const wxRect& screen_rect);
    std::string serialize() const;
};

} // namespace Slic3r::App::WX
