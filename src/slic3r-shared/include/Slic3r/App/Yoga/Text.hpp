///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class TextInternal;

class Text : public Item
{
public:
    enum class WrapMode
    {
        NoWrap, ///< No limitations for rendered text
        Wrap, ///< Text is wrapped by words, limited by width
        WrapElide ///< Text is wrapped by words, limited by width & height
    };

    explicit Text(const std::string& text, Render::ImguiFontType font_type = Render::ImguiFontType::Regular);

    const std::string& text() const;
    void set_text(const std::string& text);

    const ImColor& text_color() const;
    void set_text_color(const ImColor& text_color);

    Render::ImguiFontType font_type() const;
    void set_font_type(Render::ImguiFontType font_type);

    WrapMode wrap_mode() const;
    void set_wrap_mode(WrapMode wrap_mode);

    const Align& align() const;
    void set_align(const Align& align);

protected:
    void on_resized() override;

private:
    TextInternal* m_content_item = nullptr;
    Align m_align;
};

} // namespace Slic3r::App::Yoga
