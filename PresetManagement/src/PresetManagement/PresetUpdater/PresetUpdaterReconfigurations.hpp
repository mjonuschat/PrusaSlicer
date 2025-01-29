#ifndef slic3r_PresetUpdaterReconfigurations_hpp_
#define slic3r_PresetUpdaterReconfigurations_hpp_

#include "../../Utils/Semver.hpp"

#include <vector>
#include <string>

namespace PresetManagement {

enum class VendorReconfigurationState {
	//VUS_NO_UPDATE,
    VUS_UPDATE, // Higher version of vendor profiles exists and is compatible with SLIC3R_VERSION. Can be updated any time.
    VUS_FORCED_UPDATE, // Vendor profiles version is too low, in needs to be updated for use with SLIC3R_VERSION. Update must be done after startup. Wizard not needed. 
    VUS_FORCED_DOWNGRADE, // vendor profiles version is too high, in needs to be downgraded for use with SLIC3R_VERSION. Downgrade must be done after startup. Wizard needed. 
    VUS_CURRENT_NOT_IN_INDEX, // vendor profiles version is not in index, forced reconfiguration is needed. Reconfiguration must be done after startup. Wizard needed. 
};
struct VendorReconfiguration
{
    VendorReconfigurationState state;
    std::string vendor_id;
    std::string vendor_archive_id;
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
        , vendor_archive_id(archive_id)
	    , current_version(current_version)
        , desired_version(desired_version)
		, comment(comment)
		, changelog_url(changelog_url)
		, new_printers(new_printers)
    {}
};

class ReconfigurationsList
{
private:
    std::vector<VendorReconfiguration> m_regular_updates; //VUS_UPDATE
    std::vector<VendorReconfiguration> m_forced_updates; //VUS_FORCED_UPDATE
    std::vector<VendorReconfiguration> m_forced_downgrades; // VUS_FORCED_DOWNGRADE and VUS_CURRENT_NOT_IN_INDEX
public:
    ReconfigurationsList() = default;
    ReconfigurationsList(const ReconfigurationsList& other) = default;
    ReconfigurationsList(ReconfigurationsList&& other) noexcept = default;
    ReconfigurationsList& operator=(const ReconfigurationsList& other) = default;
    ReconfigurationsList& operator=(ReconfigurationsList&& other) noexcept = default;
    ~ReconfigurationsList() = default;

    // Method to emplace a VendorReconfiguration into the appropriate vector
    template <typename... Args>
    void emplace_back(VendorReconfigurationState state, Args&&... args) {
        switch (state) {
        case VendorReconfigurationState::VUS_UPDATE:
            m_regular_updates.emplace_back(state, std::forward<Args>(args)...);
            break;
        case VendorReconfigurationState::VUS_FORCED_UPDATE:
            m_forced_updates.emplace_back(state, std::forward<Args>(args)...);
            break;
        case VendorReconfigurationState::VUS_FORCED_DOWNGRADE:
        case VendorReconfigurationState::VUS_CURRENT_NOT_IN_INDEX:
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

}
#endif