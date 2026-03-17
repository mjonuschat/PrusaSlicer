#include "Slic3r/App/Config/ConfigItemLanguageSelection.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/App/Localization.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

ConfigItemLanguageSelection::ConfigItemLanguageSelection(
    size_t index,
    const Domain::ConfigItem& config_item,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, config_item),
    ComboBox("ConfigItemCombo"),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    set_width(150);

    m_tooltip->set_text(tooltip_text());
    m_tooltip->content_item()->set_width(350);
    m_tooltip->set_text_wrap(true);

    std::vector<std::string> names;
    auto language_infos = localization().languages();
    names.reserve(language_infos.size());

    // Some valid language should be selected since the application start up.
    const std::string active_language = localization().active_language();
    int init_selection                = -1;
    for (size_t i = 0; i < language_infos.size(); ++i) {
        if (language_infos[i].canonical_name == active_language)
            // The dictionary matches the active language and country.
            init_selection = i;
        names.emplace_back(language_infos[i].description);
    }
    set_items(names);
    set_current_index(init_selection);

    callbacks().selection_changed = [this](int selected)
    {
        m_cbi_container.set_item_value(
            *m_state,
            Domain::ConfigValue{localization().languages()[selected].canonical_name},
            m_cbi_index
        );
    };

    on_data_update();
}

void ConfigItemLanguageSelection::on_data_update()
{
    const std::string name = m_state->value().get<std::string>();

    auto language_infos = localization().languages();
    auto it             = std::find_if(
        language_infos.begin(),
        language_infos.end(),
        [&name](const Biz::LanguageShortInfo& item)
        { return item.canonical_name == name; }
    );
    ASSERT(it != language_infos.end());

    set_current_index(std::distance(language_infos.begin(), it));
}

} // namespace Slic3r::App
