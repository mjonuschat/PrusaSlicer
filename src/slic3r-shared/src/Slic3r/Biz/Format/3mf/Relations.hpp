///|/ Copyright (c) Prusa Research 2023 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Format_3mf_Relations_hpp_
#define slic3r_Format_3mf_Relations_hpp_

#include <string>
#include <optional>
#include <miniz.h> // mz_zip_archive
#include "ResultLoad3mf.hpp"

namespace Slic3r {

struct Relationship{
    std::string target; // path to target file in archive
    std::string type; // schema
};
using Relationships = std::vector<Relationship>;

// Keep data from root relationships file
struct RootRelations
{
    std::string main_model_path;
    std::string thumbnail_path;
    std::string project_file_path;
};
Relationships get_relationships(const RootRelations &root_relations);
Relationships get_relationships(const std::vector<std::string>& model_paths);

struct LoadedRelations: public ResultLoad3mf {
    std::optional<RootRelations> relations;
    int realtions_file_index = 0;
    const char *get_main_model_path() const;
public:
    using ResultLoad3mf::ResultLoad3mf; // use child constructors
    LoadedRelations(RootRelations &&relations) : relations{relations} {}
};

// 3MF Import / Export functions
void store(mz_zip_archive &archive, const Relationships &relationships, const char *filepath);

// Read relations XML from "_rels/.rels" get index of root model
LoadedRelations load_relations(mz_zip_archive &archive, const char *filepath);

} // namespace Slic3r
#endif // slic3r_Format_3mf_Relations_hpp_
