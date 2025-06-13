///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Validator;

class ComboBox : public Yoga::Item
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

    explicit ComboBox(const std::string& name = "ComboBox");
    ComboBox(
        std::initializer_list<std::string> initializer_list = {},
        const std::string& name = "ComboBox"
    );

    template<typename Container>
    ComboBox(Container&& data, const std::string& name) : ComboBox(name)
    {
        m_items.insert(
            m_items.end(), std::make_move_iterator(std::begin(data)),
            std::make_move_iterator(std::end(data))
        );
        set_current_index(0);
    }

    void render(Vec2f pos, Vec2f size) override;

    const std::vector<std::string>& items() const;
    void set_items(const std::vector<std::string>& items);

    int current_index() const;
    void set_current_index(int current_index);
    std::string current_label() const;

    bool editable() const;
    void set_editable(bool editable);

    ImGuiComboFlags flags() const;
    void set_flags(ImGuiComboFlags flags);

    Validator* validator() const;
    void set_validator(std::unique_ptr<Validator> validator);

protected:
    Vec2f get_item_size() override;

private:
    Callbacks m_callbacks;

    /**
     * holds editable field buffer of current_label
     * std::array because otherwise we would have to set up ImGui callbacks
     * in order to use std::string
     */
    std::array<char, 2048> m_buffer = {0};
    std::vector<std::string> m_items;
    int m_current_index = 0;
    std::string m_current_label;
    ImGuiComboFlags m_flags = 0;
    bool m_editable = false;
    std::unique_ptr<Validator> m_validator;
    bool m_updated = false;
};

} // namespace Slic3r::App::Yoga
