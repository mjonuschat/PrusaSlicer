#pragma once

#include <string>
#include <functional>

namespace Slic3r::App::PopNotification {

enum class PopNotificationLayout
{
    Text,
    HeaderText,
    TextButtons,
    HeaderTextButtons,
    TextProgress,
};

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

using PopNotificationLayoutVariant = std::variant<
    PopNotificationLayoutText,
    PopNotificationLayoutHeaderText,
    PopNotificationLayoutTextButtons,
    PopNotificationLayoutHeaderTextButtons,
    PopNotificationLayoutTextProgress>;

} // namespace Slic3r::App::PopNotification
