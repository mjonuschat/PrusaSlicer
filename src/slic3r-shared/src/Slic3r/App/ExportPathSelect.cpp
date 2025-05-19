#include "Slic3r/App/ExportPathSelect.hpp"
#include "Slic3r/App/IDialogManager.hpp"

namespace Slic3r::App {


void ExportPathSelect::set_wildcards(const std::string wildcard)
{
    m_wildcards = wildcard;
}

void ExportPathSelect::show_modal_dialog(const boost::filesystem::path& default_folder, const std::string& default_filename, const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback)
{
    DialogManagerSingleton::instance().get().show_save_file_dialog(
        "Export as",
        default_folder,  
        default_filename, 
        m_wildcards,
        callback);
}

GCodeExportPathSelect::GCodeExportPathSelect(bool bgcode_compatible)
    : ExportPathSelect()
{
    if (bgcode_compatible) {
        set_wildcards("G-code (*.gcode)|*.gcode|Binary G-code (*.bgcode)|*.bgcode");
    } else {
        set_wildcards(" G-code Files (*.gcode)|*.gcode");
    }
}

} // namespace  Slic3r::App