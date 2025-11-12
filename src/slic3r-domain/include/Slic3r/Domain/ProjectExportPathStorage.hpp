#pragma once

#include "Slic3r/Log.hpp"

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

namespace Slic3r::Domain {
struct ProjectExportPathStorage {
    
    boost::filesystem::path m_export_result_dir_path; 
    boost::filesystem::path m_export_project_dir_path;

    const boost::filesystem::path& export_result_dir_path() const
    {
        return m_export_result_dir_path.empty() ? m_export_project_dir_path : m_export_result_dir_path;
    }

    const boost::filesystem::path& export_project_dir_path() const
    {
        return m_export_project_dir_path;
    }

    void set_export_result_dir_path(const boost::filesystem::path& path)
    {
        boost::filesystem::path final_path = path;
        boost::system::error_code ec;
        if (!boost::filesystem::exists(path, ec)) {
            final_path = path.parent_path();
        } 
        if (boost::filesystem::is_regular_file(path, ec)) {
            final_path = path.parent_path();
        } 
        if (!boost::filesystem::exists(final_path, ec) || ec) {
            SPDLOG_ERROR("Failed to set export result path. Target directory check path: {} Error: {}", final_path.string(), ec.message());
            return;
        }
        m_export_result_dir_path = std::move(final_path);
    }

    void set_export_project_dir_path(const boost::filesystem::path& path)
    {
        boost::filesystem::path final_path(path);
        boost::system::error_code ec;
        if (!boost::filesystem::exists(path, ec)) {
            final_path = path.parent_path();
        } 
        if (boost::filesystem::is_regular_file(path, ec)) {
            final_path = path.parent_path();
        } 
        if (!boost::filesystem::exists(final_path, ec) || ec) {
            SPDLOG_ERROR("Failed to set export result path. Target directory check path: {} Error: {}", final_path.string(), ec.message());
            return;
        }
        m_export_project_dir_path = std::move(final_path);
    }
};
}