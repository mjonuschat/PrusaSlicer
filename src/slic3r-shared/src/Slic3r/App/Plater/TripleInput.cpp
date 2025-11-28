#include "Slic3r/App/Plater/TripleInput.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include <fmt/format.h>

namespace Slic3r::App::Plater {

using Yoga::InputTextField;
using Yoga::Item;
using Yoga::DoubleValidator;
using Yoga::ItemPtr;
using Yoga::Orientation;
using Yoga::Text;

static ItemPtr
small_label(const std::string& label)
{
    auto label_item{std::make_unique<Item>()};
    label_item->set_orientation(Orientation::Horizontal);
    label_item->set_width(30);
    label_item->set_height_percent(100);
    label_item->set_align_items(YGAlignCenter);
    label_item->set_justify_content(YGJustifyFlexStart);
    label_item->emplace_back<Text>(label);
    return label_item;
}

static std::pair<InputTextField*, DoubleValidator*>
emplace_coordinate_input(Item* item, const std::string& suffix)
{
    auto validator{std::make_unique<DoubleValidator>()};
    validator->set_precision(2);
    DoubleValidator* validator_ptr{validator.get()};

    auto* input{item->emplace_back<InputTextField>(
        ""
    )};
    input->set_validator(std::move(validator));
    input->set_flex_grow(1);
    input->set_default(0);

    return {input, validator_ptr};
}

TripleInput::TripleInput(const std::string& suffix, const std::vector<TripleInput::Header>& header) {
    this->set_width_percent(100);
    this->set_orientation(Yoga::Orientation::Vertical);
    this->set_gap(4);

    auto header_row{this->emplace_back<Item>()};
    header_row->set_width_percent(100);
    header_row->set_gap(10);
    for (const Header& label : header) {
        auto text{header_row->emplace_back<Text>(label.text)};
        text->set_height_percent(100);
        text->set_align_items(YGAlignCenter);
        text->set_justify_content(YGJustifyCenter);
        text->set_text_color(label.color);
        text->set_flex_grow(1);
    }
    if (header.empty()) {
        header_row->set_visible(false);
    } else {
        header_row->append(small_label(""));
    }

    auto input_row{this->emplace_back<Item>()};
    input_row->set_gap(10);
    input_row->set_width_percent(100);
    std::tie(m_input[0], m_validator[0]) = emplace_coordinate_input(input_row, suffix);
    std::tie(m_input[1], m_validator[1]) = emplace_coordinate_input(input_row, suffix);
    std::tie(m_input[2], m_validator[2]) = emplace_coordinate_input(input_row, suffix);
    input_row->append(small_label(suffix));

    for (int i{}; i < 3; ++i) {
        InputTextField* input{m_input[i]};
        input->callbacks().text_edited = [this, i]() {
            on_change(get_value(), i);
        };
    }
}

Domain::Vec3d TripleInput::get_value() const {
    return {
        m_validator[0]->value(),
        m_validator[1]->value(),
        m_validator[2]->value(),
    };
}

void TripleInput::set_value(const Domain::Vec3d& value)
{
    m_input[0]->set_text(fmt::format("{:.2f}", value.x()));
    m_input[1]->set_text(fmt::format("{:.2f}", value.y()));
    m_input[2]->set_text(fmt::format("{:.2f}", value.z()));
}

} // namespace Slic3r::App::Plater
