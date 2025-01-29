#pragma once

#include "Slic3r/Semver.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <set>
#include <string>
#include <vector>

namespace Slic3r::Biz::PresetUpdater {

enum PrinterTechnology : unsigned char
{
    // Fused Filament Fabrication
    ptFFF,
    // Stereolitography
    ptSLA,
    // Unknown, useful for command line processing
    ptUnknown,
    // Any technology, useful for parameters compatible with both ptFFF and ptSLA
    ptAny
};

class PresetUpdaterVendorProfile 
{
public:
    std::string                     name;
    std::string                     id;
    Slic3r::Semver                  config_version;
    std::string                     config_update_url;
    std::string                     changelog_url;
    std::string                     repo_id;
    std::string                     repo_prefix;
    bool                            templates_profile { false };

    struct PrinterVariant {
        PrinterVariant() {}
        PrinterVariant(const std::string &name) : name(name) {}
        std::string                 name;
    };

    struct PrinterModel {
        PrinterModel() {}
        std::string                 id;
        std::string                 name;
        PrinterTechnology           technology;
        std::string                 family;
        std::vector<PrinterVariant> variants;
        std::vector<std::string>	default_materials;
        // Vendor & Printer Model specific print bed model & texture.
        std::string 			 	bed_model;
        std::string 				bed_texture;
        std::string                 thumbnail;

        PrinterVariant*       variant(const std::string &name) {
            for (auto &v : this->variants)
                if (v.name == name)
                    return &v;
            return nullptr;
        }

        const PrinterVariant* variant(const std::string &name) const { return const_cast<PrinterModel*>(this)->variant(name); }
    };
    std::vector<PrinterModel>          models;

    std::set<std::string>              default_filaments;
    std::set<std::string>              default_sla_materials;

    PresetUpdaterVendorProfile() {}
    PresetUpdaterVendorProfile(std::string id) : id(std::move(id)) {}

    bool 		valid() const { return ! name.empty() && ! id.empty() && config_version.valid(); }

    // Load PresetUpdaterVendorProfile from an ini file.
    // If `load_all` is false, only the header with basic info (name, version, URLs) is loaded.
    static PresetUpdaterVendorProfile from_ini(const boost::filesystem::path &path, bool load_all=true);
    static PresetUpdaterVendorProfile from_ini(const boost::property_tree::ptree &tree, const boost::filesystem::path &path, bool load_all=true);

    size_t      num_variants() const { size_t n = 0; for (auto &model : models) n += model.variants.size(); return n; }
    std::vector<std::string> families() const;

    bool        operator< (const PresetUpdaterVendorProfile &rhs) const { return this->id <  rhs.id; }
    bool        operator==(const PresetUpdaterVendorProfile &rhs) const { return this->id == rhs.id; }
};

} // namespace Slic3r::Biz::PresetUpdater