///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral, Vojtěch Bubník @bubnikv
///|/ Copyright (c) 2020 Ondřej Nový @onovy
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "UpdateDialogs.hpp"

#include "PresetUpdater/PresetUpdaterReconfigurations.hpp"

#include <wx/stattext.h>
#include <wx/sizer.h>

/*
#include <cstring>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/convert.hpp>

#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/event.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/dirdlg.h>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Utils.hpp"
#include "../../Utils/AppUpdater.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "ConfigWizard.hpp"
#include "wxExtensions.hpp"
#include "format.hpp"
*/
namespace PresetManagement {

/*
static const char* URL_CHANGELOG = "https://files.prusa3d.com/?latest=slicer-stable&lng=%1%";
static const char* URL_DOWNLOAD = "https://www.prusa3d.com/slicerweb&lng=%1%";
static const char* URL_DEV = "https://github.com/prusa3d/PrusaSlicer/releases/tag/version_%1%";

static const std::string CONFIG_UPDATE_WIKI_URL("https://github.com/prusa3d/PrusaSlicer/wiki/Slic3r-PE-1.40-configuration-update");
*/



// MsgUpdateConfig

MsgUpdateConfig::MsgUpdateConfig(const ReconfigurationsList& reconfigurations) :
    MsgDialog(nullptr, "Configuration update",
					   "Configuration update is available", wxICON_ERROR)
{
    assert(!reconfigurations.regular_updates().empty());
    assert(reconfigurations.forced_updates().empty());
    assert(reconfigurations.forced_downgrades().empty());

    auto *text = new wxStaticText(this, wxID_ANY,
		"Would you like to install it?\n\n"
		"Note that a full configuration snapshot will be created first. It can then be restored at any time "
		"should there be a problem with the new version.\n\n"
		"Updated configuration bundles:"
	);
	text->Wrap(CONTENT_WIDTH * 10);
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

    auto *versions = new wxBoxSizer(wxVERTICAL);

    const std::vector<VendorReconfiguration>& updates = reconfigurations.regular_updates();
    for (const VendorReconfiguration& update : updates) {
        auto *flex = new wxFlexGridSizer(2, 0, VERT_SPACING);

		auto *text_vendor = new wxStaticText(this, wxID_ANY, update.vendor_id);
		text_vendor->SetFont(boldfont);
		flex->Add(text_vendor);
		flex->Add(new wxStaticText(this, wxID_ANY, update.desired_version.to_string()));
        /*
		if (! update.comment.empty()) {
			flex->Add(new wxStaticText(this, wxID_ANY, "Comment:"), 0, wxALIGN_RIGHT);
			auto *update_comment = new wxStaticText(this, wxID_ANY, Slic3r::GUI::from_u8(update.comment));
			update_comment->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			flex->Add(update_comment);
		}

		if (! update.new_printers.empty()) {
			flex->Add(new wxStaticText(this, wxID_ANY, _L_PLURAL("New printer", "New printers", update.new_printers.find(',') == std::string::npos ? 1 : 2) + ":"), 0, wxALIGN_RIGHT);
			auto* update_printer = new wxStaticText(this, wxID_ANY, from_u8(update.new_printers));
			update_printer->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			flex->Add(update_printer);
		}

		versions->Add(flex);

		if (! update.changelog_url.empty() && update.version.prerelease() == nullptr) {
			auto *line = new wxBoxSizer(wxHORIZONTAL);
			auto changelog_url = (boost::format(update.changelog_url) % lang_code).str();
			line->AddSpacer(3*VERT_SPACING);
			line->Add(new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open changelog page")), changelog_url));
			versions->Add(line);
			versions->AddSpacer(1); // empty value for the correct alignment inside a GridSizer
		}
        */
    }

    content_sizer->Add(versions);
	content_sizer->AddSpacer(2*VERT_SPACING);

    add_button(wxID_OK, true, "Yes");
	add_button(wxID_CANCEL, false, "No");

    finalize();
}

/*
MsgUpdateConfig::MsgUpdateConfig(const std::vector<Update> &updates, PresetUpdater::UpdateParams update_params) :
	MsgDialog(nullptr, update_params == PresetUpdater::UpdateParams::FORCED_BEFORE_WIZARD  ? _L("Opening Configuration Wizard") : _L("Configuration update"), 
					   update_params == PresetUpdater::UpdateParams::FORCED_BEFORE_WIZARD ? _L("PrusaSlicer is not using the newest configuration available.\n"
												"Configuration Wizard may not offer the latest printers, filaments and SLA materials to be installed.") : 
											 _L("Configuration update is available"), wxICON_ERROR)
{
	auto *text = new wxStaticText(this, wxID_ANY, _(L(
		"Would you like to install it?\n\n"
		"Note that a full configuration snapshot will be created first. It can then be restored at any time "
		"should there be a problem with the new version.\n\n"
		"Updated configuration bundles:"
	)));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	const auto lang_code = wxGetApp().current_language_code_safe().ToStdString();

	auto *versions = new wxBoxSizer(wxVERTICAL);
	for (const auto &update : updates) {
		auto *flex = new wxFlexGridSizer(2, 0, VERT_SPACING);

		auto *text_vendor = new wxStaticText(this, wxID_ANY, update.vendor);
		text_vendor->SetFont(boldfont);
		flex->Add(text_vendor);
		flex->Add(new wxStaticText(this, wxID_ANY, update.version.to_string()));

		if (! update.comment.empty()) {
			flex->Add(new wxStaticText(this, wxID_ANY, _(L("Comment:"))), 0, wxALIGN_RIGHT);
			auto *update_comment = new wxStaticText(this, wxID_ANY, from_u8(update.comment));
			update_comment->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			flex->Add(update_comment);
		}

		if (! update.new_printers.empty()) {
			flex->Add(new wxStaticText(this, wxID_ANY, _L_PLURAL("New printer", "New printers", update.new_printers.find(',') == std::string::npos ? 1 : 2) + ":"), 0, wxALIGN_RIGHT);
			auto* update_printer = new wxStaticText(this, wxID_ANY, from_u8(update.new_printers));
			update_printer->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			flex->Add(update_printer);
		}

		versions->Add(flex);

		if (! update.changelog_url.empty() && update.version.prerelease() == nullptr) {
			auto *line = new wxBoxSizer(wxHORIZONTAL);
			auto changelog_url = (boost::format(update.changelog_url) % lang_code).str();
			line->AddSpacer(3*VERT_SPACING);
			line->Add(new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open changelog page")), changelog_url));
			versions->Add(line);
			versions->AddSpacer(1); // empty value for the correct alignment inside a GridSizer
		}
	}

	content_sizer->Add(versions);
	content_sizer->AddSpacer(2*VERT_SPACING);

    if (update_params == PresetUpdater::UpdateParams::FORCED_BEFORE_WIZARD) {
        add_button(wxID_OK, true,  _L("Install"));
        auto* btn = add_button(wxID_CLOSE, false, _L("Don't install"));
		btn->Bind(wxEVT_BUTTON, [this](const wxCommandEvent&) { this->EndModal(wxID_CLOSE); });
        add_button(wxID_CANCEL);
    } else if (update_params == PresetUpdater::UpdateParams::SHOW_TEXT_BOX_YES_NO) {
        add_button(wxID_OK, true, _L("Yes"));
	    add_button(wxID_CANCEL, false, _L("No"));
    } else {
        assert(update_params == PresetUpdater::UpdateParams::SHOW_TEXT_BOX);
        add_button(wxID_OK, true, "OK");
	    add_button(wxID_CANCEL);
    }

	finalize();
}
*/

MsgUpdateConfig::~MsgUpdateConfig() {}

/*
//MsgUpdateForced

MsgUpdateForced::MsgUpdateForced(const std::vector<Update>& updates) :
    MsgDialog(nullptr, wxString::Format(_(L("%s incompatibility")), SLIC3R_APP_NAME), _(L("You must install a configuration update.")) + " ", wxOK | wxICON_ERROR)
{
	auto* text = new wxStaticText(this, wxID_ANY, wxString::Format(_(L(
		"%s will now start updates. Otherwise it won't be able to start.\n\n"
		"Note that a full configuration snapshot will be created first. It can then be restored at any time "
		"should there be a problem with the new version.\n\n"
		"Updated configuration bundles:"
	)), SLIC3R_APP_NAME));
	

	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	const auto lang_code = wxGetApp().current_language_code_safe().ToStdString();

	auto* versions = new wxFlexGridSizer(2, 0, VERT_SPACING);
	for (const auto& update : updates) {
		auto* text_vendor = new wxStaticText(this, wxID_ANY, update.vendor);
		text_vendor->SetFont(boldfont);
		versions->Add(text_vendor);
		versions->Add(new wxStaticText(this, wxID_ANY, update.version.to_string()));

		if (!update.comment.empty()) {
			versions->Add(new wxStaticText(this, wxID_ANY, _(L("Comment:"))));//uncoment if align to right (might not look good if 1  vedor name is longer than other names)
			auto* update_comment = new wxStaticText(this, wxID_ANY, from_u8(update.comment));
			update_comment->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			versions->Add(update_comment);
		}

		if (!update.new_printers.empty()) {
			versions->Add(new wxStaticText(this, wxID_ANY, _L_PLURAL("New printer", "New printers", update.new_printers.find(',') == std::string::npos ? 1 : 2)+":"));
			auto* update_printer = new wxStaticText(this, wxID_ANY, from_u8(update.new_printers));
			update_printer->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			versions->Add(update_printer);
		}

		if (!update.changelog_url.empty() && update.version.prerelease() == nullptr) {
			auto* line = new wxBoxSizer(wxHORIZONTAL);
			auto changelog_url = (boost::format(update.changelog_url) % lang_code).str();
			line->AddSpacer(3 * VERT_SPACING);
			line->Add(new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open changelog page")), changelog_url));
			versions->Add(line);
			versions->AddSpacer(1); // empty value for the correct alignment inside a GridSizer
		}
	}

	content_sizer->Add(versions);
	content_sizer->AddSpacer(2 * VERT_SPACING);

	add_button(wxID_EXIT, false, wxString::Format(_L("Exit %s"), SLIC3R_APP_NAME));
	for (auto ID : { wxID_EXIT, wxID_OK })
		get_button(ID)->Bind(wxEVT_BUTTON, [this](const wxCommandEvent& evt) { this->EndModal(evt.GetId()); });

	finalize();
}

MsgUpdateForced::~MsgUpdateForced() {}

// MsgDataIncompatible

MsgDataIncompatible::MsgDataIncompatible(const std::unordered_map<std::string, wxString> &incompats) :
    MsgDialog(nullptr, wxString::Format(_(L("%s incompatibility")), SLIC3R_APP_NAME), 
                       wxString::Format(_(L("%s configuration is incompatible")), SLIC3R_APP_NAME), wxICON_ERROR)
{
	auto *text = new wxStaticText(this, wxID_ANY, wxString::Format(_(L(
		"This version of %s is not compatible with currently installed configuration bundles.\n"
		"This probably happened as a result of running an older %s after using a newer one.\n\n"

		"You may either exit %s and try again with a newer version, or you may re-run the initial configuration. "
		"Doing so will create a backup snapshot of the existing configuration before installing files compatible with this %s.")) + "\n", 
		SLIC3R_APP_NAME, SLIC3R_APP_NAME, SLIC3R_APP_NAME, SLIC3R_APP_NAME));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);

	auto *text2 = new wxStaticText(this, wxID_ANY, wxString::Format(_(L("This %s version: %s")), SLIC3R_APP_NAME, SLIC3R_VERSION));
	text2->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text2);
	content_sizer->AddSpacer(VERT_SPACING);

	auto *text3 = new wxStaticText(this, wxID_ANY, _(L("Incompatible bundles:")));
	text3->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text3);
	content_sizer->AddSpacer(VERT_SPACING);

	auto *versions = new wxFlexGridSizer(2, 0, VERT_SPACING);
	for (const auto &incompat : incompats) {
		auto *text_vendor = new wxStaticText(this, wxID_ANY, incompat.first);
		text_vendor->SetFont(boldfont);
		versions->Add(text_vendor);
		versions->Add(new wxStaticText(this, wxID_ANY, incompat.second));
	}

	content_sizer->Add(versions);
	content_sizer->AddSpacer(2*VERT_SPACING);

	add_button(wxID_REPLACE, true, _L("Re-configure"));
	add_button(wxID_EXIT, false, wxString::Format(_L("Exit %s"), SLIC3R_APP_NAME));

	for (auto ID : {wxID_EXIT, wxID_REPLACE})
		get_button(ID)->Bind(wxEVT_BUTTON, [this](const wxCommandEvent& evt) { this->EndModal(evt.GetId()); });

	finalize();
}

MsgDataIncompatible::~MsgDataIncompatible() {}


// MsgDataLegacy

MsgDataLegacy::MsgDataLegacy() :
	MsgDialog(nullptr, _(L("Configuration update")), _(L("Configuration update")))
{
    auto *text = new wxStaticText(this, wxID_ANY, format_wxstr( _L(
			"%s now uses an updated configuration structure.\n\n"

			"So called 'System presets' have been introduced, which hold the built-in default settings for various "
			"printers. These System presets cannot be modified, instead, users now may create their "
			"own presets inheriting settings from one of the System presets.\n"
			"An inheriting preset may either inherit a particular value from its parent or override it with a customized value.\n\n"

			"Please proceed with the %s that follows to set up the new presets "
			"and to choose whether to enable automatic preset updates."
        )
        , SLIC3R_APP_NAME, ConfigWizard::name()));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	auto *text2 = new wxStaticText(this, wxID_ANY, _(L("For more information please visit our wiki page:")));
	static const wxString url("https://github.com/prusa3d/PrusaSlicer/wiki/Slic3r-PE-1.40-configuration-update");
	// The wiki page name is intentionally not localized:
	// TRN %s = PrusaSlicer
	auto *link = new wxHyperlinkCtrl(this, wxID_ANY, format_wxstr(_L("%s 1.40 configuration update"), SLIC3R_APP_NAME), CONFIG_UPDATE_WIKI_URL);
	content_sizer->Add(text2);
	content_sizer->Add(link);
	content_sizer->AddSpacer(VERT_SPACING);

	finalize();
}

MsgDataLegacy::~MsgDataLegacy() {}


// MsgNoUpdate

MsgNoUpdates::MsgNoUpdates() :
    MsgDialog(nullptr, _(L("Configuration updates")), _(L("No updates available")), wxICON_ERROR | wxOK)
{

	auto* text = new wxStaticText(this, wxID_ANY, wxString::Format(
		_(L(
            "%s has no configuration updates available."
		)),
        SLIC3R_APP_NAME
	));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	finalize();
}

MsgNoUpdates::~MsgNoUpdates() {}

// MsgNoAppUpdates
MsgNoAppUpdates::MsgNoAppUpdates() :
	MsgDialog(nullptr, _(L("App update")), _(L("No updates available")), wxICON_ERROR | wxOK)
{
	//TRN %1% is PrusaSlicer
	auto* text = new wxStaticText(this, wxID_ANY, format_wxstr(_L("Your %1% is up to date."),SLIC3R_APP_NAME));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	finalize();
}

MsgNoAppUpdates::~MsgNoAppUpdates() {}
*/

}
