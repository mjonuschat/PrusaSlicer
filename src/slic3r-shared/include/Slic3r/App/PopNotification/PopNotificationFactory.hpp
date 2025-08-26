#pragma once

#include "Slic3r/App/PopNotification/PopNotificationData.hpp"

#include <string>
#include <chrono>
#include <limits>

using namespace std::chrono;

namespace Slic3r::App::PopNotification {

class PopNotificationFactory
{
public:
    static size_t next_id()
    {
        return s_next_id++;
    }

    static PopNotificationDataPtr create_custom(PopNotificationLevel level, int duration_seconds, const std::string& text)
    {
        return std::make_unique<PopNotificationData>(s_next_id++, PopNotificationType::Custom, level, duration_seconds, PopNotificationLayout::Text, PopNotificationLayoutText(text));
    }

    static PopNotificationDataPtr
    create_text(PopNotificationType type, PopNotificationLevel level, int duration_seconds, const std::string& text)
    {
        return std::make_unique<PopNotificationData>(s_next_id++, type, level, duration_seconds, PopNotificationLayout::Text, PopNotificationLayoutText(text));
    }

    static PopNotificationDataPtr
    create_custom_with_header(PopNotificationLevel level, int duration_seconds, const std::string& text, const std::string& header)
    {
        return std::make_unique<PopNotificationData>(s_next_id++, PopNotificationType::Custom, level, duration_seconds, PopNotificationLayout::HeaderText, PopNotificationLayoutHeaderText(header, text));
    }

    static PopNotificationDataPtr create_button_test()
    {
        PopNotificationButtonData b1("button 1", []() { return true; });
        std::vector<PopNotificationButtonData> buttons =
            {std::move(b1), {"button 2", []() { return false; }}};

        return std::make_unique<PopNotificationData>(
            s_next_id++,
            PopNotificationType::Custom,
            PopNotificationLevel::Important,
            0,
            PopNotificationLayout::HeaderTextButtons,
            PopNotificationLayoutHeaderTextButtons("This must be important right?", "First button will close this notification.", std::move(buttons))
        );
    }

    static PopNotificationDataPtr create_progress_test()
    {
        return std::make_unique<PopNotificationData>(s_next_id++, PopNotificationType::Custom, PopNotificationLevel::Important, 0, PopNotificationLayout::TextProgress, PopNotificationLayoutTextProgress("Progress bar:", 25));
    }

    static PopNotificationDataPtr create_error_test()
    {
        return std::make_unique<PopNotificationData>(
            s_next_id++,
            PopNotificationType::Custom,
            PopNotificationLevel::Error,
            0,
            PopNotificationLayout::HeaderTextButtons,
            PopNotificationLayoutHeaderTextButtons(
                "ERROR",
                "Something is wrong.",
                {{"solve all the problems", []() { return true; }}}
            )
        );
    }

    static PopNotificationDataPtr
    create_job_notification(const std::string& text, const std::string& job_name, const Slic3r::Biz::Platform::JobManager::Progress& progress)
    {
        return std::make_unique<
            PopNotificationData>(s_next_id++, PopNotificationType::JobProgress, PopNotificationLevel::Important, 0, PopNotificationLayout::Text, PopNotificationLayoutText(text), JobProgressNotificationData(job_name, progress));
    }

private:
    inline static size_t s_next_id = 0;
};

} // namespace Slic3r::App::PopNotification
