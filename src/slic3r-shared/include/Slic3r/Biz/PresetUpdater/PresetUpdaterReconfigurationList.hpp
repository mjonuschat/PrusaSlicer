#pragma once

#include "Slic3r/Semver.hpp"
#include <string>

namespace Slic3r::Biz::PresetUpdater {

enum class VendorReconfigurationState {
    Update, // Higher version of vendor profiles exists and is compatible with SLIC3R_VERSION. Can be updated any time.
    ForcedUpdate, // Vendor profiles version is too low, in needs to be updated for use with SLIC3R_VERSION. Update must be done after startup. Wizard not needed. 
    ForcedDowngrade, // vendor profiles version is too high, in needs to be downgraded for use with SLIC3R_VERSION. Downgrade must be done after startup. Wizard needed. 
    NotInIndex, // vendor profiles version is not in index, forced reconfiguration is needed. Reconfiguration must be done after startup. Wizard needed. 
};
struct VendorReconfiguration
{
    VendorReconfigurationState state;
    std::string vendor_id;
    std::string vendor_repo_id;
    Slic3r::Semver current_version;
    Slic3r::Semver desired_version;
    std::string comment;
	std::string changelog_url;
    std::string new_printers;
    VendorReconfiguration(
        VendorReconfigurationState s, 
        const std::string& id, 
        const std::string& archive_id, 
        const Slic3r::Semver& current_version, 
        const Slic3r::Semver& desired_version, 
        const std::string& comment, 
        const std::string& changelog_url, 
        const std::string& new_printers)
        :  state(s)
        , vendor_id(id)
        , vendor_repo_id(archive_id)
	    , current_version(current_version)
        , desired_version(desired_version)
		, comment(comment)
		, changelog_url(changelog_url)
		, new_printers(new_printers)
    {}
};

class PresetUpdaterReconfigurationList
{
private:
    std::vector<VendorReconfiguration> m_regular_updates; //Update
    std::vector<VendorReconfiguration> m_forced_updates; //ForcedUpdate
    std::vector<VendorReconfiguration> m_forced_downgrades; // ForcedDowngrade and NotInIndex
public:
    PresetUpdaterReconfigurationList() = default;
    PresetUpdaterReconfigurationList(const PresetUpdaterReconfigurationList& other) = default;
    PresetUpdaterReconfigurationList(PresetUpdaterReconfigurationList&& other) noexcept = default;
    PresetUpdaterReconfigurationList& operator=(const PresetUpdaterReconfigurationList& other) = default;
    PresetUpdaterReconfigurationList& operator=(PresetUpdaterReconfigurationList&& other) noexcept = default;
    ~PresetUpdaterReconfigurationList() = default;

    // Method to emplace a VendorReconfiguration into the appropriate vector
    template <typename... Args>
    void emplace_back(VendorReconfigurationState state, Args&&... args) {
        switch (state) {
        case VendorReconfigurationState::Update:
            m_regular_updates.emplace_back(state, std::forward<Args>(args)...);
            break;
        case VendorReconfigurationState::ForcedUpdate:
            m_forced_updates.emplace_back(state, std::forward<Args>(args)...);
            break;
        case VendorReconfigurationState::ForcedDowngrade:
        case VendorReconfigurationState::NotInIndex:
            m_forced_downgrades.emplace_back(state, std::forward<Args>(args)...);
            break;
        default:
            assert(false);
            break;
        }
    }

    const std::vector<VendorReconfiguration>& regular_updates() const { return m_regular_updates; }
    const std::vector<VendorReconfiguration>& forced_updates() const { return m_forced_updates; }
    const std::vector<VendorReconfiguration>& forced_downgrades() const { return m_forced_downgrades; }
};


} // namespace Slic3r::Biz::PresetUpdater