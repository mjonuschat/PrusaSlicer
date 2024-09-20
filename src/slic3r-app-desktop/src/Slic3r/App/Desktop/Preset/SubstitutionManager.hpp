///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <vector>
#include <string>

#include "libslic3r/Preset.hpp"
#include "Slic3r/Biz/Preset/IConfigInteractor.hpp"

class wxFlexGridSizer;
class wxWindow;
class wxString;

namespace Slic3r::App::Desktop::Preset {

// G-code substitutions

// Substitution Manager - helper for manipuation of the substitutions
class SubstitutionManager
{
    Biz::Preset::IConfigInteractor* m_config_interactor{nullptr};
    wxWindow*			m_parent{ nullptr };
	wxFlexGridSizer*	m_grid_sizer{ nullptr };

	int                 m_em{10};
	std::function<void()> m_cb_edited_substitution{ nullptr };
	std::function<void()> m_cb_hide_delete_all_btn{ nullptr };

	std::vector<std::string>	m_substitutions;
	std::vector<wxWindow*>		m_chb_match_single_lines;

	void validate_length();
	bool is_compatible_with_ui();
	bool is_valid_id(int substitution_id, const wxString& message);

public:
	SubstitutionManager() = default;
	~SubstitutionManager() = default;

	void init(Biz::Preset::IConfigInteractor* config_interactor, wxWindow* parent, wxFlexGridSizer* grid_sizer);
	void create_legend();
	void delete_substitution(int substitution_id);
	void add_substitution(	int substitution_id = -1,
							const std::string& plain_pattern = std::string(),
							const std::string& format = std::string(),
							const std::string& params = std::string(),
							const std::string& notes  = std::string());
	void update_from_config();
	void delete_all();
	void edit_substitution(int substitution_id, 
						   int opt_pos, // option position insubstitution [0, 2]
						   const std::string& value);
	void set_cb_edited_substitution(std::function<void()> cb_edited_substitution) {
		m_cb_edited_substitution = cb_edited_substitution;
	}
	void call_ui_update() {
		if (m_cb_edited_substitution)
			m_cb_edited_substitution();
	}
	void set_cb_hide_delete_all_btn(std::function<void()> cb_hide_delete_all_btn) {
		m_cb_hide_delete_all_btn = cb_hide_delete_all_btn;
	}
	void hide_delete_all_btn() {
		if (m_cb_hide_delete_all_btn)
			m_cb_hide_delete_all_btn();
	}
	bool is_empty_substitutions();

private:
    const DynamicPrintConfig& config() const { return m_config_interactor->config(); }
};

} 
