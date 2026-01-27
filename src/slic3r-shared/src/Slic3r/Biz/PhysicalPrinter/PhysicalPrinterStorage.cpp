#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterStorage.hpp"

#include "Slic3r/Biz/Config/ConfigLoad.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Assert.hpp"
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/nowide/fstream.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PhysicalPrinter {

namespace {
inline fs::path storage_dir()
{
    static const fs::path dir = fs::path{data_dir()} / "physical_printer";
    return dir;
}
} // namespace

PhysicalPrinterStorage::PhysicalPrinterStorage()
{
    load_all();
}

PhysicalPrinterStorage::~PhysicalPrinterStorage()
{
}

void PhysicalPrinterStorage::load_all() 
{
    const fs::path dir_path = storage_dir();
    boost::system::error_code ec;
    fs::directory_iterator it(dir_path, ec);
    
    if (ec) {
        SPDLOG_ERROR("Failed to load Physical Printer Configurations: {}", ec.message());
        return;
    }

    for (const auto& entry : it) {
        std::ifstream file(entry.path().string());
        if (!file.is_open()) continue;

        nlohmann::ordered_json json;
        std::string uuid;
        try {
            file >> json;
            uuid = json.at("physical_printer_uuid").get<std::string>();
            if (uuid.empty()) {
                SPDLOG_ERROR("Physical printer configuration was not loaded due missing uuid. The file will be deleted.");
                continue;
            }
        } catch (...) {
            SPDLOG_ERROR("Failed to read physical printer from json file: {}", entry.path().string());
            continue;
        }

        Domain::PhysicalPrinterSettings settings;
        Biz::Config::load_box(json, settings);
        m_map[uuid] = std::move(settings);
    }

    consolidate_files();
}

void PhysicalPrinterStorage::consolidate_files()
{
    const fs::path dir_path = storage_dir();
    boost::system::error_code ec;
    fs::directory_iterator it(dir_path, ec);
    
    if (ec) {
        SPDLOG_ERROR("Failed to consolidate Physical Printer Configurations: {}", ec.message());
        return;
    }

    bool needs_save = false;
    for (const auto& entry : it) {
        std::string current_stem = entry.path().stem().string();
        if (m_map.find(current_stem) == m_map.end()) {
            fs::remove(entry.path(), ec);
            needs_save = true;
        }
    }
    if (needs_save) {
        save_all();
    }
}


void PhysicalPrinterStorage::save_one(const std::string& uuid)
{
    auto it = m_map.find(uuid);
    if (it == m_map.end()) {
        SPDLOG_ERROR("Attempted to save non-existent physical printer uuid: {}", uuid);
        return;
    }

    const fs::path dir_path = storage_dir();
    boost::system::error_code ec;
    fs::create_directories(dir_path, ec);
    if (ec) {
        SPDLOG_ERROR("Failed to create directory for Physical Printers: {}", ec.message());
        return;
    }

    fs::path full_path = dir_path / (uuid + ".json");
    boost::nowide::ofstream out(full_path.string());

    if (!out.is_open()) {
        SPDLOG_ERROR("Failed to open file for writing: {}", full_path.string());
        return;
    }
    
    try {
        nlohmann::ordered_json json = it->second;
        out << json.dump(2);
    } catch (...) {
        SPDLOG_ERROR("Failed to write physical printer in json file: {}", full_path.string());
    }
}

void PhysicalPrinterStorage::remove_one(const std::string& uuid)
{
    auto it = m_map.find(uuid);
    if (it == m_map.end()) {
        SPDLOG_ERROR("Attempted to remove non-existent physical printer uuid: {}", uuid);
        return;
    }

    m_map.erase(it);

    const fs::path file_path = storage_dir() / (uuid + ".json");
    boost::system::error_code ec;
    
    fs::remove(file_path, ec);
    if (ec) {
        SPDLOG_ERROR("Failed to delete physical printer file {}: {}", file_path.string(), ec.message());
    }
}


void PhysicalPrinterStorage::save_all()
{
    for (const auto& [uuid, settings] : m_map) {
        save_one(uuid);
    } 
}

Domain::PhysicalPrinterSettings& PhysicalPrinterStorage::printer_settings(const std::string& filename)
{
    return m_map[filename];
}

void PhysicalPrinterStorage::add_printer_settings(Domain::PhysicalPrinterSettings&& settings, const std::string& filename)
{
    m_map[filename] = std::move(settings);
    save_all();
}

const UuidSettingsMap& PhysicalPrinterStorage::all_settings() const
{
    return m_map;
}

UuidSettingsMap& PhysicalPrinterStorage::all_settings()
{
    return m_map;
}

Domain::PhysicalPrinterSettings& PhysicalPrinterStorage::dummy_settings()
{
    return m_dummy_settings;
}

std::string PhysicalPrinterStorage::store_dummy(const std::string& model, const std::string& base_model)
{
    std::string uuid{boost::uuids::to_string(boost::uuids::random_generator()())};
    m_dummy_settings.find("physical_printer_uuid").item->set<std::string>(uuid);
    m_dummy_settings.find("physical_printer_preset_model").item->set<std::string>(model);
    m_dummy_settings.find("physical_printer_preset_base_model").item->set<std::string>(base_model);
    m_map[uuid] = std::move(m_dummy_settings);
    m_dummy_settings = Domain::PhysicalPrinterSettings();
    save_all();
    return uuid;
}

} //namespace Slic3r::Biz::PhysicalPrinter
