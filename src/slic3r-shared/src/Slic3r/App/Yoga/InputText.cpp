///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/InputText.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"

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
    cb_user_data.Str                   = str;
    cb_user_data.ChainCallback         = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputTextEx(
        label,
        hint,
        str->data(),
        str->capacity() + 1,
        size,
        flags,
        InputTextCallback,
        &cb_user_data
    );
}

InputText::InputText(const std::string& name)
{
    set_item_name(name.empty() ? "InputText" : name);
}

void InputText::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    {
        Imgui::ScopedStyleColors colors({{ImGuiCol_FrameBg, IM_COL32_BLACK_TRANS}});
        ImGui::SetCursorScreenPos(to_im(pos));
        const std::string id = "###" + m_item_name;

        ImU32 text_color = ImGui::GetColorU32(enabled() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);

        ImGui::PushFont(m_imgui_render->font(m_font_type));

        ImGuiID input_id = ImGui::GetID(id.c_str());

        // Query before rendering
        bool activated = ImGui::GetActiveID() == input_id;
        bool changed   = false;

        if (m_request_focus) {
            m_request_focus = false;
            ImGui::SetKeyboardFocusHere();
        }

        if (!activated && !m_override_label.empty()) {
            // We are overriding hint label
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, text_color);
            std::string empty;
            changed = YInputText(
                id.c_str(),
                m_override_label.c_str(),
                &empty,
                to_im(size),
                (m_flags | (enabled() ? 0 : ImGuiInputTextFlags_ReadOnly)),
                {},
                nullptr
            );
            ImGui::PopStyleColor();
        } else {
            changed = YInputText(
                id.c_str(),
                m_hint.c_str(),
                &m_text,
                to_im(size),
                (m_flags | (enabled() ? 0 : ImGuiInputTextFlags_ReadOnly)),
                {},
                nullptr
            );
        }

        ImGui::PopFont();

        ImGui::PopStyleColor();

        m_updated |= changed;

        if (m_updated) {
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                m_updated   = false;
                m_has_focus = false;
                if (m_validator) {
                    m_text = m_validator->process(m_text);
                }
                if (m_callbacks.update_revert_button) {
                    m_callbacks.update_revert_button();
                }
                if (m_callbacks.text_edited) {
                    m_callbacks.text_edited();
                }
                if (m_callbacks.focus_lost) {
                    m_callbacks.focus_lost();
                }
            }

            if (ImGui::IsItemEdited() && m_callbacks.text_changed) {
                m_callbacks.text_changed();
            }
        }

        if (m_has_focus && ImGui::IsItemDeactivated()) {
            m_has_focus = false;
            if (m_callbacks.focus_lost) {
                m_callbacks.focus_lost();
            }
        }
        if (!m_has_focus && ImGui::IsItemActivated()) {
            m_has_focus = true;
            if (m_callbacks.focus_gained) {
                m_callbacks.focus_gained();
            }
        }

        if (m_active
            && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            && m_callbacks.text_entered)
        {
            m_callbacks.text_entered();
        }

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        m_active            = ImGui::GetActiveID() == window->GetID(id.c_str());
    }

    render_item_end(pos, size);
}

InputText::Callbacks& InputText::callbacks()
{
    return m_callbacks;
}

const std::string& InputText::text() const
{
    return m_text;
}

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

ImGuiInputTextFlags InputText::flags() const
{
    return m_flags;
}

void InputText::set_flags(ImGuiInputTextFlags flags)
{
    m_flags = flags;
}

const std::string& InputText::hint() const
{
    return m_hint;
}

void InputText::set_hint(const std::string& hint)
{
    m_hint = hint;
}

Validator* InputText::validator() const
{
    return m_validator.get();
}

void InputText::set_validator(std::unique_ptr<Validator> validator)
{
    m_validator = std::move(validator);
}

bool InputText::active() const
{
    return m_active;
}

bool InputText::has_focus() const
{
    return m_has_focus;
}

void InputText::request_focus()
{
    m_request_focus = true;
    set_style_dirty(); // request a new render loop
}

Vec2f InputText::get_item_size()
{
    return {50, ImGui::GetTextLineHeight() + GImGui->Style.FramePadding.y * 2.0f};
}

const std::string& InputText::override_label() const
{
    return m_override_label;
}

void InputText::set_override_label(const std::string& override_label)
{
    m_override_label = override_label;
}

Render::ImguiFontType InputText::font_type() const
{
    return m_font_type;
}

void InputText::set_font_type(Render::ImguiFontType font_type)
{
    m_font_type = font_type;
}

} // namespace Slic3r::App::Yoga
