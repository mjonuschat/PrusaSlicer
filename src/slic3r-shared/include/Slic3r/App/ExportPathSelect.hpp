#pragma once

#include <boost/filesystem/path.hpp>
#include <vector>

namespace Slic3r::App {

class ExportPathSelect {
public:
    ExportPathSelect() = default;
    virtual ~ExportPathSelect() = default;

    /**
     *  @brief Sets wildcard for modal dialog. Needs to be called before show_modal_dialog.
     */
    void set_wildcards(const std::string wildcard);

    /**
     * @brief Shows modal dialog "Save file as". Returns true if user confirmed selection.
     */
    void show_modal_dialog(
        const boost::filesystem::path& default_folder,
        const std::string& default_filename,
        const std::function<void(bool result, const std::vector<boost::filesystem::path>& file_paths)>& callback
    );

private:
    std::string m_wildcards;
};

class GCodeExportPathSelect final : public  ExportPathSelect
{
public:

    /**
     * @brief Calls ExportPathSelect constructor and set_wildcards with gcode / bgcode.
     */
    GCodeExportPathSelect(bool bgcode_compatible);
};

} // namespace  Slic3r::App