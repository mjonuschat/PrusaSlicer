///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "DiffNamespace.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/Widgets/CheckBox.hpp"
#include "Slic3r/App/WX/I18N.hpp"

#include <Slic3r/Assert.hpp>

namespace Slic3r::App::WX::Diff {

BCB::BCB(
    wxWindow* parent,
    std::function<void(int selection, Location location)> fn,
    Location location
) :
    Widgets::BitmapComboBox(
        parent,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(30 * w_config()->em_unit(), 0),
        0,
        NULL,
        wxCB_READONLY
    ),
    m_select_fn(fn),
    m_location(location)
{
    this->Bind(
        wxEVT_COMBOBOX,
        [this](wxCommandEvent& evt)
        {
            int selection = evt.GetSelection();
            if (m_select_fn) {
                m_select_fn(selection, m_location);
            }
        }
    );

    this->Bind(
        wxEVT_SIZE,
        [this](wxSizeEvent& evt)
        {
            evt.Skip();
            this->Refresh();
        }
    );
}

void BCB::SetSelection(int n)
{
    if (this->HasClientObjectData()) {
        BCBClientData* obj = dynamic_cast<BCBClientData*>(this->GetClientObject(n));
        if (obj && obj->is_marker()) {
            ASSERT(m_old_selection >= 0);
            this->SetSelection(m_old_selection);
            return;
        }
    }

    Widgets::BitmapComboBox::SetSelection(n);
    m_old_selection = n;
}

Row::Row(
    wxWindow* parent_win,
    std::function<void(int selection, Location location)> fn,
    Row* depends_on_row
) :
    wxBoxSizer(wxHORIZONTAL),
    m_select_fn(fn),
    m_depends_on_row(depends_on_row)
{
    const int em = w_config()->em_unit();

    m_checkbox = new Widgets::CheckBox(parent_win);
    m_checkbox->SetValue(true);
    m_checkbox->SetToolTip(
        _L("When enabled, the difference for this preset type is displayed in the tree below.")
    );

    m_left  = new BCB(parent_win, fn, Location::Left);
    m_right = new BCB(parent_win, fn, Location::Right);

    m_equal_bmp = new ScalableButton(parent_win, wxID_ANY, "equal");
    m_equal_bmp->Bind(
        wxEVT_BUTTON,
        [this](wxEvent&)
        {
            if (state != State::NotEqual) {
                // there is nothing to equalize
                return;
            }

            // We can equalize right to left, only when m_depends_on_row.state == State::Equal
            // OR this row doesn't have m_depends_on_row 
            // OR m_depends_on_row is hidden
            if (!m_depends_on_row || !m_depends_on_row->IsShown(1) || m_depends_on_row->state == State::Equal) {
                int left_selection = m_left->GetSelection();
                m_right->SetSelection(left_selection);
                if (m_select_fn) {
                    m_select_fn(left_selection, Location::Right);
                }
            }
        }
    );

    this->Add(m_checkbox);
    this->Add(m_left, 1, wxEXPAND | wxLEFT | wxRIGHT, em);
    this->Add(m_equal_bmp);
    this->Add(m_right, 1, wxEXPAND | wxLEFT, em);
}

BCB* Row::operator[](Location location)
{
    return location == Location::Left ? m_left : m_right;
}

BCBClientData* Row::client_data(int selection, Location location)
{
    BCB* cb = (*this)[location];
    if (cb->HasClientObjectData()) {
        return dynamic_cast<BCBClientData*>(cb->GetClientObject(selection));
    }
    return nullptr;
}

BCBClientData* Row::client_data_selected(Location location)
{
    return client_data(selection(location), location);
}

int Row::selection(Location location)
{
    return (*this)[location]->GetSelection();
}

void Row::select(int selection, Location location)
{
    (*this)[location]->SetSelection(selection);
}

void Row::set_state(State new_state)
{
    if (new_state != State::Undef) {
        // check comboboxes for the content
        if (m_left->IsEmpty() || m_right->IsEmpty()) {
            // if some of them is empty => it's Undefind state
            new_state = State::Undef;
        }
    }

    state = new_state;

    m_equal_bmp->SetBitmap_(
        state == State::Equal        ? "equal" :
            state == State::NotEqual ? "not_equal" :
                                       "empty"
    );

    wxString tooltip = wxEmptyString;
    if (state == State::NotEqual) {
        tooltip = _L("Presets are different.");
        // We can equalize right to left, only when m_depends_on_row.state == State::Equal or this row doesn't have m_depends_on_row;
        if (m_depends_on_row == nullptr || m_depends_on_row->state == State::Equal) {
            tooltip += from_u8("\n")
                + _L("Click this button to select the same preset for the right and left preset.");
        } else {
            tooltip += from_u8("\n")
                + _L("All previous settings should be equal to equalize them\n"
                     "(to select the same preset for the right and left preset)");
        }
    }
    m_equal_bmp->SetToolTip(tooltip);

    m_checkbox->Enable(new_state != State::Undef);
    m_left->Enable(new_state != State::Undef);
    m_right->Enable(new_state != State::Undef);
}

void Row::set_checkbox_callback(std::function<void()> fn)
{
    m_checkbox->Bind(wxEVT_CHECKBOX, [fn](wxCommandEvent&) { fn(); });
}

bool Row::is_checked_checkbox() const
{
    return m_checkbox->GetValue();
}

void Row::show_checkbox(bool show)
{
    m_checkbox->Show(show);
}

} // namespace Slic3r::App::WX::Diff
