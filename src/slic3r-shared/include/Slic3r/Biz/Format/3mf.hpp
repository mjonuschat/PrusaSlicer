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

namespace Slic3r {

    // Forward decalrations
    namespace Domain { class Model; }
    struct ConfigSubstitutionContext;
    class DynamicPrintConfig;
    struct ThumbnailData;

    // Returns true if the 3mf file with the given filename is a PrusaSlicer project file (i.e. if it contains a config).
    [[deprecated("It is solved inside of load 3mf.")]] 
    extern bool is_project_3mf(const std::string& filename);

    /// <summary>
    /// Load 3mf file into the given model and preset bundle.
    /// </summary>
    /// <param name="path">Filepath to 3mf file(or zip archive) in UTF8</param>
    /// <param name="config">Configuration for printing</param>
    /// <param name="config_substitutions">Helper class for forward/backward
    /// configuration compatibility</param>
    /// <param name="model">[Output] write loaded model</param>
    /// <param name="check_version"> ??? </param>
    /// <returns>True on success otherwise false</returns>
    extern bool load_3mf(
        std::string_view filepath_3mf,
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
        bool use_production_extension = true;

        // stored uncompressed 3mf - better versioning of uncompressed data
        // "0 - The file is stored (no compression)" in accordance with the OPC specification
        // ("Annex C, (normative) ZIP Appnote.txt Clarifications
        bool use_uncompressed_version = false; // Not implemented yet

        CT_Metadata_Model metadata;
    };

    /// <summary>
    /// Save model geometry and configuration into 3mf file.
    /// NOTE: COULD throw exception boost::filesystem::filesystem_error
    /// </summary>
    /// <param name="filepath">Filename with path for store 3mf file in UTF8</param>
    /// <param name="model">Model of scene geometry and orientation
    /// <param name="config">Configuration for printing</param>
    extern void store_3mf(
        const std::string &filepath,
        const Model &model, 
        const DynamicPrintConfig* config, const Store3mfParam &param = Store3mfParam{});

} // namespace Slic3r

#endif /* slic3r_Format_3mf_hpp_ */
