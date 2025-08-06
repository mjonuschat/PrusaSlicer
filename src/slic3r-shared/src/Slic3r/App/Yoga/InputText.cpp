///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/InputText.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include <Slic3r/Log.hpp>

namespace Slic3r::App::Yoga {

struct InputTextCallback_UserData
{
    std::string* Str;
    ImGuiInputTextCallback ChainCallback;
    void* ChainCallbackUserData;
};

/**
 * @note copied from imgui_stdlib as we would like to have a backend in std::string
 */
static int InputTextCallback(ImGuiInputTextCallbackData* data)
{
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*) data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we
        // need to set them back to what we want.
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = str->data();
    } else if (user_data->ChainCallback) {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

/**
 * @note copied from imgui_stdlib as we would like to have a backend in std::string, but also
 * include size argument
 */
static bool YInputText(
    const char* label,
    const char* hint,
    std::string* str,
    ImVec2 size,
    ImGuiInputTextFlags flags,
    ImGuiInputTextCallback callback,
    void* user_data
)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextEx(
        label, hint, str->data(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data
    );
}

InputText::InputText(const std::string& name) { set_item_name(name); }

void InputText::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    {
        Imgui::ScopedStyleColors colors({{ImGuiCol_FrameBg, IM_COL32_BLACK_TRANS}});
        ImGui::SetCursorScreenPos(to_im(pos));
        const std::string id = "###" + m_item_name;

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImGui::GetColorU32(enabled() ? ImGuiCol_Text : ImGuiCol_TextDisabled)
        );

        bool changed = YInputText(
            id.c_str(),
            m_hint.c_str(),
            &m_text,
            to_im(size),
            (m_flags | (enabled() ? 0 : ImGuiInputTextFlags_ReadOnly)),
            {},
            nullptr
        );
        ImGui::PopStyleColor();

        m_updated |= changed;

        if (m_updated) {
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                m_updated = false;
                if (m_validator) {
                    m_text = m_validator->process(m_text);
                }
                if (m_callbacks.update_revert_button) {
                    m_callbacks.update_revert_button();
                }
                if (m_callbacks.text_edited) {
                    m_callbacks.text_edited();
                }
            }
            if (changed && m_callbacks.text_changed) {
                m_callbacks.text_changed();
            }
        }

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        m_active = ImGui::GetActiveID() == window->GetID(id.c_str());
    }

    render_item_end(pos, size);
}

InputText::Callbacks& InputText::callbacks() { return m_callbacks; }

const std::string& InputText::text() const { return m_text; }

void InputText::set_text(const std::string& text)
{
    if (m_text == text) {
        return;
    }

    if (validator()) {
        m_text = validator()->process(text);
    } else {
        m_text = text;
    }
    if (m_callbacks.update_revert_button) {
        m_callbacks.update_revert_button();
    }
    if (m_callbacks.text_changed) {
        m_callbacks.text_changed();
    }
}

ImGuiInputTextFlags InputText::flags() const { return m_flags; }

void InputText::set_flags(ImGuiInputTextFlags flags) { m_flags = flags; }

const std::string& InputText::hint() const { return m_hint; }

void InputText::set_hint(const std::string& hint) { m_hint = hint; }

Validator* InputText::validator() const { return m_validator.get(); }

void InputText::set_validator(std::unique_ptr<Validator> validator)
{
    m_validator = std::move(validator);
}

bool InputText::active() const
{
    return m_active;
}

Vec2f InputText::get_item_size()
{
    return {50, ImGui::GetTextLineHeight() + GImGui->Style.FramePadding.y * 2.0f};
}

} // namespace Slic3r::App::Yoga
