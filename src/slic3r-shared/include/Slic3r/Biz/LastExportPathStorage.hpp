#pragma once

#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz {
struct LastExportPathStorage {

    boost::filesystem::path last_export_path_local; 
    boost::filesystem::path last_export_path_removable;

    boost::filesystem::path get_last_export_path(bool only_removable) const {
        return only_removable ? last_export_path_removable : last_export_path_local;
    }
    void set_last_export_path(const boost::filesystem::path& path, bool is_removable) {
        if (is_removable) {
            last_export_path_removable = path;
        } else {
            last_export_path_local = path;
        }
    }
};
}