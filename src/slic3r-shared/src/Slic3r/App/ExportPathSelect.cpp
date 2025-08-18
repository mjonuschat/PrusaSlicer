#include "Slic3r/App/ExportPathSelect.hpp"
#include "Slic3r/App/AppServices.hpp"

namespace Slic3r::App {

void ExportPathSelect::set_wildcards(const std::string wildcard)
{
    m_wildcards = wildcard;
}

void ExportPathSelect::show_modal_dialog(const boost::filesystem::path& default_folder, const std::string& default_filename, const IDialogManager::FileCallback& callback)
{
    AppServices::instance().dialog_manager().show_file_dialog(FileDialogType::Save, "Export as", default_folder, default_filename, m_wildcards, callback);
}

GCodeExportPathSelect::GCodeExportPathSelect(bool bgcode_compatible) : ExportPathSelect()
{
    if (bgcode_compatible) {
        set_wildcards("G-code (*.gcode)|*.gcode|Binary G-code (*.bgcode)|*.bgcode");
    } else {
        set_wildcards(" G-code Files (*.gcode)|*.gcode");
    }
}

} // namespace  Slic3r::App
