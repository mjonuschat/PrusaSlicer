#include "Slic3r/App/OverrideItemPreviewRow.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Config/ConfigItemPreview.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

OverrideItemPreviewRow::OverrideItemPreviewRow(
    size_t index,
    const Biz::OverrideItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Biz::OverrideItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    set_gap(10);
    set_padding({0, 0, 10, 0});
    set_flex_shrink(0);
    set_object_name("OverrideItemPreviewRow");
    set_height(26);
    set_align_items(YGAlignCenter);

    m_label = emplace_back<Text>(std::string());
    m_label->set_width(170);
    m_label->set_wrap_mode(Text::WrapMode::Wrap);

    Item* container = emplace_back<Item>();
    container->set_gap(3);
    container->set_flex_grow(1);
    container->set_align_items(YGAlignCenter);

    m_preview = container->emplace_back<ConfigItemPreview>();
    m_preview->set_text_font_type(Render::ImguiFontType::Italic);

    m_sidetext = container->emplace_back<Text>(std::string());
    m_sidetext->set_flex_grow(1);
    m_sidetext->set_wrap_mode(Text::WrapMode::WrapElide);
    m_sidetext->set_font_type(Render::ImguiFontType::Italic);

    m_add_button = container->emplace_back<LayoutButton>(
        std::string(),
        Render::Icon::Plus,
        Biz::_u8L("Add override")
    );
    m_add_button->set_background_color(Platform::Color::ButtonTransparent);
    m_add_button->set_icon_tint(
        m_theme->color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled)
    );
    m_add_button->set_width(22);
    m_add_button->set_height(22);
    m_add_button->set_content_padding(Paddings(2));
    m_add_button->set_flex_shrink(0);
    m_add_button->callbacks().action = [this]
    { m_preset_interactor.set_item_override(*m_state->config_item, true); };

    on_data_update();
}

void OverrideItemPreviewRow::on_data_update()
{
    ASSERT(m_state->is_override());

    m_label->set_text(Biz::_u8(m_state->config_item->def().label));
    m_preview->set_data(*m_state->config_item, m_state->config_item->value(), false);

    m_add_button->set_visible(!m_state->overriden.value());

    m_sidetext->set_text(
        *m_state->config_item->def().type == typeid(Domain::FloatOrPercentage) ?
            std::string() : // For this item, m_preview already contains the measurement unit.
            Biz::_u8(m_state->config_item->def().sidetext)
    );
}

} // namespace Slic3r::App
