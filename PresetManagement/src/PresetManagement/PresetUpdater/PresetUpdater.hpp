///|/ Copyright (c) Prusa Research 2018 - 2023 David Kocík @kocikdav, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Oleksandra Iushchenko @YuSanka, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_PresetUpdater_hpp_
#define slic3r_PresetUpdater_hpp_


#include <map>
#include <string>
#include <vector>

namespace PresetManagement {

class ReconfigurationsList;
class PresetUpdaterProcessStatus;
struct VendorReconfiguration;

class PresetUpdater
{
public:
	PresetUpdater() = default;
	PresetUpdater(PresetUpdater &&) = delete;
	PresetUpdater(const PresetUpdater &) = delete;
	PresetUpdater& operator=(PresetUpdater &&) = delete;
	PresetUpdater& operator=(const PresetUpdater &) = delete;
	~PresetUpdater() = default;


    void check_forced_reconfigurations(ReconfigurationsList& results, PresetUpdaterProcessStatus* process_status) const;
    void check_reconfigurations(ReconfigurationsList& results, PresetUpdaterProcessStatus* process_status) const;
    void perform_reconfigurations(const ReconfigurationsList& reconfigurations, PresetUpdaterProcessStatus* process_status) const;
private:
    void perform_downgrades(const std::vector<VendorReconfiguration>& downgrades, PresetUpdaterProcessStatus* process_status) const;
    void perform_updates(const std::vector<VendorReconfiguration>& updates, PresetUpdaterProcessStatus* process_status) const;
};
}
#endif
