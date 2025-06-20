///|/ Copyright (c) Prusa Research 2024 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Format_3mf_hpp_
#define slic3r_Format_3mf_hpp_

#include <string>
#include <string_view>
#include <vector>
#include <limits>
#include "Slic3r/Biz/Format/Metadata.hpp"
#include "Slic3r/Biz/Format/ResultLoad3mf.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Project.hpp"

namespace Slic3r {

    // Forward decalrations
    namespace Domain { class Model; }
    struct ConfigSubstitutionContext;
    class DynamicPrintConfig;
    struct ThumbnailData;

    class Old3MFException : public std::exception {};

    bool load_3mf(const std::string& file_path, Domain::Project& project); // Dummy function (temp)

    bool load_3mf(
        const std::string& filepath_3mf,
        DynamicPrintConfig &config,
        ConfigSubstitutionContext &config_substitutions,
        Domain::Model &model,
        bool check_version
    );

    /// <summary>
    /// Settings and flags parameters for different save into 3mf file
    /// </summary>
    struct Store3mfParam{
        // Publish option to hide imported local file path
        bool fullpath_sources = true;

        // Preview for stored geometry 
        // Used as file icon of the 3mf file by OPC
        // NOTE: In future it will be generated inside of store function 
        const ThumbnailData *thumbnail_data = nullptr;

        // Flag to force using of the zip64 compression function
        bool zip64 = true;

        // Use https://github.com/3MFConsortium/spec_production/releases/tag/1.2.0
        // to store huge model geometries into separated files (faster store/load)
        bool use_production_extension = false;

        // stored uncompressed 3mf - better versioning of uncompressed data
        // "0 - The file is stored (no compression)" in accordance with the OPC specification
        // ("Annex C, (normative) ZIP Appnote.txt Clarifications
        bool use_uncompressed_version = false; // Not implemented yet

        CT_Metadata_Model metadata;
    };


    void store_3mf(
        const std::string &filepath,
        const Domain::Project& project,
        const Store3mfParam &param = Store3mfParam{});

} // namespace Slic3r

#endif /* slic3r_Format_3mf_hpp_ */
