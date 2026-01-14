#pragma once

#include <string>
#include <functional>

namespace Slic3r::Biz {

class IMessageDialogProvider
{
public:
    using YesNoCallback           = std::function<void(bool answer)>;
    using CheckBoxCheckedCallback = std::function<void(bool checked)>;

    IMessageDialogProvider()          = default;
    virtual ~IMessageDialogProvider() = default;

    virtual void show_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const YesNoCallback& callback
    ) = 0;
    virtual void show_rich_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const std::string& check_text,
        const YesNoCallback& callback,
        const CheckBoxCheckedCallback& checked_callback
    ) = 0;
    virtual void
    show_info_dialog(const std::string& text, const std::string& title = std::string(), bool is_marked = false) = 0;
    virtual void
    show_warning_dialog(const std::string& text, const std::string& title = std::string()) = 0;
    virtual void
    show_error_dialog(const std::string& text, const std::string& title = std::string()) = 0;

    virtual void show_load_step_dialog(const std::string& filename) = 0;
};

} // namespace Slic3r::Biz
