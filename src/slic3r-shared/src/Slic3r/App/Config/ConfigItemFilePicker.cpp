///|/ Copyright (c) Prusa Research 2026 Oleksandra Iushchenko @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemFilePicker.hpp"
#include "Slic3r/App/Yoga/InputText.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include "Slic3r/App/IDialogManager.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

// Temporary workaround based on current internal agreement.
// TODO: Replace with a data-driven solution.
static std::string get_wildcards(const std::string& config_item_name)
{
    if (config_item_name == "bed_custom_texture") {
        return Wildcards::generate_wildcards(
            Wildcards::TypeFlag::Svg | Wildcards::TypeFlag::Png | Wildcards::TypeFlag::AllTextures,
            Wildcards::TypeFlag::AllTextures
        );
    }

    if (config_item_name == "bed_custom_model") {
        return Wildcards::generate_wildcards(Wildcards::TypeFlag::Stl);
    }
    return std::string();
}

ConfigItemFilePicker::ConfigItemFilePicker(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    Vec2f btn_size{24, 24};
    set_gap(10.f);

    m_tooltip = emplace_back<Tooltip>(this, tooltip_text(), std::string{});
    m_tooltip->set_text_wrap(true);
    m_tooltip->content_item()->set_width(350);

    m_file_name = emplace_back<InputText>("FileName");
    m_file_name->set_flags(ImGuiInputTextFlags_ReadOnly);
    m_file_name->set_font_type(Render::ImguiFontType::Bold);
    m_file_name->set_flex_grow(1.f);
    m_file_name->callbacks().hovered_changed = [this](bool hovered)
    { hovered ? m_tooltip->open() : m_tooltip->close(); };

    m_load_btn = emplace_back<LayoutButton>("", Render::Icon::TobBarLoad);
    m_load_btn->set_min_width(btn_size.x());
    m_load_btn->set_min_height(btn_size.y());
    m_load_btn->callbacks().action = [this]()
    {
        IDialogManager::FileCallback callback =
            [this](bool success, const std::vector<boost::filesystem::path>& file_paths)
        {
            if (success) {
                std::string file_name = file_paths.begin()->filename().string();
                if (m_file_name->text() != file_name) {
                    m_file_name->set_text(file_name);

                    m_cbi_container.set_item_value(
                        *m_state,
                        Domain::ConfigValue{file_paths.begin()->string()},
                        m_cbi_index
                    );
                }

                m_remove_btn->set_visible(true);
            }
        };
        App::AppServices::instance().dialog_manager().show_file_dialog(
            FileDialogType::Open,
            Biz::_u8L("Load file"),
            AppServices::instance().app_config().get<std::string>("last_used_directory"),
            "",
            get_wildcards(m_state->name()),
            callback
        );
    };

    m_remove_btn = emplace_back<LayoutButton>("", Render::Icon::DeleteBtnIcon);
    m_remove_btn->set_min_width(btn_size.x());
    m_remove_btn->set_min_height(btn_size.y());
    m_remove_btn->callbacks().action = [this]()
    {
        m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{""}, m_cbi_index);
        m_remove_btn->set_visible(false);
    };

    on_data_update();
}

void ConfigItemFilePicker::on_data_update()
{
    const std::string init_val = m_state->get<std::string>();
    if (init_val.empty()) {
        m_file_name->set_text(Biz::_u8L("None"));
    } else {
        boost::filesystem::path path(init_val);
        std::string file_name = path.filename().string();
        if (m_file_name->text() != file_name) {
            m_file_name->set_text(file_name);
        }
    }
    m_remove_btn->set_visible(!init_val.empty());
}

} // namespace Slic3r::App
