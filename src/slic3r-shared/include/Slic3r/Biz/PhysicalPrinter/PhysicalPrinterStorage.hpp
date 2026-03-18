#pragma once

#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

#include <string>
#include <map>


namespace Slic3r::Domain::Preset {
struct HwPrinterConfig;
}

namespace Slic3r::Biz::PhysicalPrinter {

using UuidSettingsMap = std::map<std::string, Domain::PhysicalPrinterSettings>;
using UuidHwConfigMap = std::map<std::string, Domain::Preset::HwPrinterConfig>;

class PhysicalPrinterStorage
{
public:
    PhysicalPrinterStorage();
    ~PhysicalPrinterStorage();

    void load_all();
    void save_all();
    void save_one(const std::string& uuid, const Domain::Preset::HwPrinterConfig& hw_config);
    void remove_one(const std::string& uuid);

    Domain::PhysicalPrinterSettings& printer_settings(const std::string& filename);
    void add_printer_settings(Domain::PhysicalPrinterSettings&& settings, const std::string& filename);

    const UuidSettingsMap& all_settings() const;

    UuidSettingsMap& all_settings();

    Domain::PhysicalPrinterSettings& dummy_settings();

    std::string create_from_dummy(const Domain::Preset::HwPrinterConfig& hw_config);

    const Domain::Preset::HwPrinterConfig& hw_config(const std::string& uuid) const
    {
        return m_hw_config_supplement_map.at(uuid);
    }

private:
    void consolidate_files();

private:
    UuidSettingsMap m_map; 
    UuidHwConfigMap m_hw_config_supplement_map;
    Domain::PhysicalPrinterSettings m_dummy_settings;
};

} //namespace Slic3r::Biz::PhysicalPrinter
