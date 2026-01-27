#pragma once

#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

#include <string>
#include <map>

namespace Slic3r::Biz::PhysicalPrinter {

using UuidSettingsMap = std::map<std::string, Domain::PhysicalPrinterSettings>;

class PhysicalPrinterStorage
{
public:
    PhysicalPrinterStorage();
    ~PhysicalPrinterStorage();

    void load_all();
    void save_all();
    void save_one(const std::string& uuid);
    void remove_one(const std::string& uuid);

    Domain::PhysicalPrinterSettings& printer_settings(const std::string& filename);
    void add_printer_settings(Domain::PhysicalPrinterSettings&& settings, const std::string& filename);

    const UuidSettingsMap& all_settings() const;

    UuidSettingsMap& all_settings();

    Domain::PhysicalPrinterSettings& dummy_settings();

    std::string store_dummy(const std::string& model, const std::string& base_model);

private:
    void consolidate_files();

private:
    UuidSettingsMap m_map; 
    Domain::PhysicalPrinterSettings m_dummy_settings;
};

} //namespace Slic3r::Biz::PhysicalPrinter
