///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Validator;

/**
 * @brief The InputText class is a text input class which intentionally provides
 * NO decoration. The class itself is intended to be included in composed input
 * blocks such as InputTextField or InputTextArea etc.
 * The reasoning behind is that we can use Yoga layout while decorating text
 * input fields.
 */
class InputText : public Item
{
public:
    struct Callbacks
    {
        /**
         * @brief text_edited is fired only after editing is finished (e.g. Enter/ESC or item lost
         * it's focus) This is due to optional validator which is invoked just before this callback
         */
        std::function<void()> text_edited{nullptr};
    };

    explicit InputText(const std::string& name = "InputText");

    void render(Vec2f pos, Vec2f size) override;

    Callbacks& callbacks();

    /**
     * @note We assume UTF-8 encoding
     */
    const std::string& text() const;
    void set_text(const std::string& text);

    ImGuiInputTextFlags flags() const;
    void set_flags(ImGuiInputTextFlags flags);

    const std::string& hint() const;
    void set_hint(const std::string& hint);

    Validator* validator() const;
    void set_validator(std::unique_ptr<Validator> validator);

    /**
     * @return true if InputText is currently active (focused/edited)
     */
    bool active() const;

private:
    Callbacks m_callbacks;

    /**
     * @note IntValidator and DoubleValidator are intended to be used with
     * ImGuiInputTextFlags_CharsDecimal you need to set that flag manually
     */
    std::unique_ptr<Validator> m_validator;

    std::string m_text;
    ImGuiInputTextFlags m_flags = 0;
    std::string m_hint;

    bool m_active = false;
    bool m_updated = false; ///< Input was edited but not yet lost focus

    // Item interface
protected:
    Vec2f get_item_size() override;
};

} // namespace Slic3r::App::Yoga
