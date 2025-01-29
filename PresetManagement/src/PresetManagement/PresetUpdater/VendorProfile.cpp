#include "VendorProfile.hpp"

#include <boost/nowide/fstream.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

using boost::property_tree::ptree;

namespace PresetManagement {
VendorProfile VendorProfile::from_ini(const boost::filesystem::path &path, bool load_all)
{
    ptree tree;
    boost::nowide::ifstream ifs(path.string());
    boost::property_tree::read_ini(ifs, tree);
    return VendorProfile::from_ini(tree, path, load_all);
}

static const std::unordered_map<std::string, std::string> pre_family_model_map {{
    { "MK3",        "MK3" },
    { "MK3MMU2",    "MK3" },
    { "MK2.5",      "MK2.5" },
    { "MK2.5MMU2",  "MK2.5" },
    { "MK2S",       "MK2" },
    { "MK2SMM",     "MK2" },
    { "SL1",        "SL1" },
}};

VendorProfile VendorProfile::from_ini(const ptree &tree, const boost::filesystem::path &path, bool load_all)
{
    static const std::string printer_model_key = "printer_model:";
    static const std::string filaments_section = "default_filaments";
    static const std::string materials_section = "default_sla_materials";

    const std::string id = path.stem().string();

    if (! boost::filesystem::exists(path)) {
        throw Slic3r::RuntimeError((boost::format("Cannot load Vendor Config Bundle `%1%`: File not found: `%2%`.") % id % path).str());
    }

    VendorProfile res(id);

    // Helper to get compulsory fields
    auto get_or_throw = [&](const ptree &tree, const std::string &key) -> ptree::const_assoc_iterator
    {
        auto res = tree.find(key);
        if (res == tree.not_found()) {
            throw Slic3r::RuntimeError((boost::format("Vendor Config Bundle `%1%` is not valid: Missing secion or key: `%2%`.") % id % key).str());
        }
        return res;
    };

    // Load the header
    const auto &vendor_section = get_or_throw(tree, "vendor")->second;
    res.name = get_or_throw(vendor_section, "name")->second.data();

    auto config_version_str = get_or_throw(vendor_section, "config_version")->second.data();
    auto config_version = Slic3r::Semver::parse(config_version_str);
    if (! config_version) {
        throw Slic3r::RuntimeError((boost::format("Vendor Config Bundle `%1%` is not valid: Cannot parse config_version: `%2%`.") % id % config_version_str).str());
    } else {
        res.config_version = std::move(*config_version);
    }

    // Load URLs
    const auto config_update_url = vendor_section.find("config_update_url");
    if (config_update_url != vendor_section.not_found()) {
        res.config_update_url = config_update_url->second.data();
    }

    const auto changelog_url = vendor_section.find("changelog_url");
    if (changelog_url != vendor_section.not_found()) {
        res.changelog_url = changelog_url->second.data();
    }

    const auto templates_profile = vendor_section.find("templates_profile");
    if (templates_profile != vendor_section.not_found()) {
        res.templates_profile = templates_profile->second.data() == "1";
    }

    const auto repo_id = vendor_section.find("repo_id");
    if (repo_id != vendor_section.not_found()) {
        res.repo_id = repo_id->second.data();
    } else {
        // For backward compatibility assume all profiles without repo_id are from "prod" repo
        // DK: "No, dont!"
        res.repo_id = "";
    }

    const auto repo_prefix = vendor_section.find("repo_prefix");
    if (repo_prefix != vendor_section.not_found()) {
         res.repo_prefix = repo_prefix->second.data();
    } else {
        res.repo_prefix = "";
    }

    if (! load_all) {
        return res;
    }

    return res;
}

std::vector<std::string> VendorProfile::families() const
{
    std::vector<std::string> res;
    unsigned num_familiies = 0;

    for (auto &model : models) {
        if (std::find(res.begin(), res.end(), model.family) == res.end()) {
            res.push_back(model.family);
            num_familiies++;
        }
    }

    return res;
}
} // PresetManagement