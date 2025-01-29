///|/ Copyright (c) Prusa Research 2018 - 2023 David Kocík @kocikdav, Lukáš Hejl @hejllukas, Oleksandra Iushchenko @YuSanka, Vojtěch Král @vojtechkral, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_UpdateDialogs_hpp_
#define slic3r_UpdateDialogs_hpp_

#include "../Utils/Semver.hpp"
#include "../Utils/MsgDialog.hpp"

#include <string>
#include <vector>
#include <wx/hyperlink.h>

/*
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "libslic3r/Semver.hpp"
#include "MsgDialog.hpp"

#include "slic3r/Utils/PresetUpdater.hpp"
*/
class wxBoxSizer;
class wxCheckBox;


namespace PresetManagement {

class ReconfigurationsList;

// Confirmation dialog informing about configuration update. Lists updated bundles & their versions.
class MsgUpdateConfig : public Slic3r::GUI::MsgDialog
{
public:
    /*
	struct Update
	{
		std::string vendor;
		Slic3r::Semver version;
		std::string comment;
		std::string changelog_url;
		std::string new_printers;

		Update(std::string vendor, Slic3r::Semver version, std::string comment, std::string changelog_url, std::string new_printers)
			: vendor(std::move(vendor))
			, version(std::move(version))
			, comment(std::move(comment))
			, changelog_url(std::move(changelog_url))
			, new_printers(std::move(new_printers))
		{}
	};
    */
	// force_before_wizard - indicates that check of updated is forced before ConfigWizard opening
	//MsgUpdateConfig(const std::vector<Update> &updates, PresetUpdater::UpdateParams update_params);
    MsgUpdateConfig(const ReconfigurationsList& reconfigurations);
    MsgUpdateConfig(MsgUpdateConfig &&) = delete;
	MsgUpdateConfig(const MsgUpdateConfig &) = delete;
	MsgUpdateConfig &operator=(MsgUpdateConfig &&) = delete;
	MsgUpdateConfig &operator=(const MsgUpdateConfig &) = delete;
	~MsgUpdateConfig();
};

/*
// Informs about currently installed bundles not being compatible with the running Slic3r. Asks about action.
class MsgUpdateForced : public MsgDialog
{
public:
	struct Update
	{
		std::string vendor;
		Semver version;
		std::string comment;
		std::string changelog_url;
		std::string new_printers;

		Update(std::string vendor, Semver version, std::string comment, std::string changelog_url, std::string new_printers)
			: vendor(std::move(vendor))
			, version(std::move(version))
			, comment(std::move(comment))
			, changelog_url(std::move(changelog_url))
			, new_printers(std::move(new_printers))
		{}
	};

	MsgUpdateForced(const std::vector<Update>& updates);
	MsgUpdateForced(MsgUpdateForced&&) = delete;
	MsgUpdateForced(const MsgUpdateForced&) = delete;
	MsgUpdateForced& operator=(MsgUpdateForced&&) = delete;
	MsgUpdateForced& operator=(const MsgUpdateForced&) = delete;
	~MsgUpdateForced();
};

// Informs about currently installed bundles not being compatible with the running Slic3r. Asks about action.
class MsgDataIncompatible : public MsgDialog
{
public:
	// incompats is a map of "vendor name" -> "version restrictions"
	MsgDataIncompatible(const std::unordered_map<std::string, wxString> &incompats);
	MsgDataIncompatible(MsgDataIncompatible &&) = delete;
	MsgDataIncompatible(const MsgDataIncompatible &) = delete;
	MsgDataIncompatible &operator=(MsgDataIncompatible &&) = delete;
	MsgDataIncompatible &operator=(const MsgDataIncompatible &) = delete;
	~MsgDataIncompatible();
};

// Informs about a legacy data directory - an update from Slic3r PE < 1.40
class MsgDataLegacy : public MsgDialog
{
public:
	MsgDataLegacy();
	MsgDataLegacy(MsgDataLegacy &&) = delete;
	MsgDataLegacy(const MsgDataLegacy &) = delete;
	MsgDataLegacy &operator=(MsgDataLegacy &&) = delete;
	MsgDataLegacy &operator=(const MsgDataLegacy &) = delete;
	~MsgDataLegacy();
};

// Informs about absence of bundles requiring update.
class MsgNoUpdates : public MsgDialog
{
public:
	MsgNoUpdates();
	MsgNoUpdates(MsgNoUpdates&&) = delete;
	MsgNoUpdates(const MsgNoUpdates&) = delete;
	MsgNoUpdates& operator=(MsgNoUpdates&&) = delete;
	MsgNoUpdates& operator=(const MsgNoUpdates&) = delete;
	~MsgNoUpdates();
};

// Informs about absence of new version online.
class MsgNoAppUpdates : public MsgDialog
{
public:
	MsgNoAppUpdates();
	MsgNoAppUpdates(MsgNoAppUpdates&&) = delete;
	MsgNoAppUpdates(const MsgNoAppUpdates&) = delete;
	MsgNoAppUpdates& operator=(MsgNoUpdates&&) = delete;
	MsgNoAppUpdates& operator=(const MsgNoAppUpdates&) = delete;
	~MsgNoAppUpdates();
};
*/
}

#endif
