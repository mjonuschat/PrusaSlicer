#pragma once

#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {
class Text;
class LayoutButton;
class ProgressBar;
class Icon;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::PopNotification {

class PopNotificationView : public Yoga::Window, public Biz::DataObserver<PopNotificationData>
{
public:
    PopNotificationView(size_t index, const PopNotificationData& data, PopNotificationObservableList& notification_list);

protected:
    void on_data_update() override;

private:
    // const PopNotificationData& m_notification_data;
    PopNotificationObservableList& m_notification_list;

    Yoga::Item* m_left_column;
    Yoga::Item* m_mid_column;
    Yoga::Item* m_right_column;

    Yoga::Icon* m_left_icon{nullptr};
    Yoga::Text* m_text{nullptr};
    Yoga::Text* m_header{nullptr};
    std::vector<Yoga::LayoutButton*> m_buttons;
    Yoga::Item* m_button_line;
    Yoga::Text* m_progress_percent_text{nullptr};
    Yoga::ProgressBar* m_progress_bar;

    PopNotificationLayout m_current_layout;
    PopNotificationLevel m_current_level;

    void reset();
    void layout();

    void basic_layout(Render::Icon icon_override);
    void basic_left_layout(Render::Icon icon_override);
    
    void basic_mid_layout();
    void basic_mid_header_layout(const std::string& header);
    void basic_mid_text_layout(const std::string& text);
    void basic_mid_buttons_layout(const std::vector<PopNotificationButtonData>& buttons);
    void basic_mid_progress_layout(int progress);

    void layout_type_text();
    void layout_type_header_text();
    void layout_type_text_buttons();
    void layout_type_header_text_buttons();
    void layout_type_text_progress();

    void update_text(const std::string& text);
    void update_header(const std::string& text);
    void update_buttons(const std::vector<PopNotificationButtonData>& buttons);
    void update_progress(int progress);

    ImColor text_color();
    ImColor button_color();
};
} // namespace Slic3r::App::PopNotification
