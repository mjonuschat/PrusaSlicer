#pragma once

#include <string>
#include <functional>
#include <variant>

namespace Slic3r::App::PopNotification {

struct PopNotificationButtonData
{
    std::string text;
    std::function<bool(void)> callback; // returns true if notification should close.
};

struct PopNotificationLayoutText
{
    std::string text;
};

struct PopNotificationLayoutHeaderText
{
    std::string header;
    std::string text;
};

struct PopNotificationLayoutTextButtons
{
    std::string text;
    std::vector<PopNotificationButtonData> buttons;
};

struct PopNotificationLayoutHeaderTextButtons
{
    std::string header;
    std::string text;
    std::vector<PopNotificationButtonData> buttons;
};

struct PopNotificationLayoutTextProgress
{
    std::string text;
    int progress;
};

struct PopNotificationLayoutHeaderTextProgress
{
    std::string header;
    std::string text;
    int progress;
};

using PopNotificationLayout = std::variant<
    PopNotificationLayoutText,
    PopNotificationLayoutHeaderText,
    PopNotificationLayoutTextButtons,
    PopNotificationLayoutHeaderTextButtons,
    PopNotificationLayoutTextProgress,
    PopNotificationLayoutHeaderTextProgress>;

} // namespace Slic3r::App::PopNotification
