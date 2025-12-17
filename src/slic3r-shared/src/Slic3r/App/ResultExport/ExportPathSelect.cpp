#include "Slic3r/App/ResultExport/ExportPathSelect.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/AppConfig.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ResultExport/ExportNameParser.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

#include "boost/filesystem/path.hpp"

using Slic3r::Biz::ExportNameParser::Technology;
using Slic3r::Biz::ExportNameParser::ExportNameData;

namespace Slic3r::App::ExportPathSelect {

namespace {

// Shortens placeholder parser error in case there is an arrow pointing to some variable.
// This is done because lines are very short in notification and it breaks the pretty error outcome.
// example:
// Parsing error at line 1: Not a variable name
// {input_filename_base}_{layer_height}mm_{printing_filament_types}_{printer_model}_{print_timeabcde}.gcode
//                                                                                   ^
// Parsing error at line 1: Not a variable name
// {print_timeabcde}

std::string shorten_error(const std::string& errorMsg) {
    std::istringstream stream(errorMsg);
    std::string header, content, pointer;
    
    // Parse the three lines
    if (!std::getline(stream, header) || 
        !std::getline(stream, content) || 
        !std::getline(stream, pointer)) {
        return errorMsg; 
    }

    size_t arrowPos = pointer.find('^');
    if (arrowPos == std::string::npos) return errorMsg;

    arrowPos = std::min(arrowPos, content.length() - 1);
    size_t start = content.rfind('{', arrowPos);    
    size_t end = (start != std::string::npos) ? content.find('}', start) : std::string::npos;
    if (start != std::string::npos && end != std::string::npos) {
        // Return Header + Newline + Variable token only
        return header + "\n" + content.substr(start, end - start + 1);
    }
    return errorMsg;
}

std::string gen_wildcards(const std::string& extension, Technology tech)
{
    if (tech == Technology::Fdm) {
        if (extension == ".bgcode" || extension == ".BGCODE") {
            return Wildcards::generate_wildcards(Wildcards::TypeFlag::BinaryGCode | Wildcards::TypeFlag::GCode, Wildcards::TypeFlag::BinaryGCode);  
        }
        return Wildcards::generate_wildcards(Wildcards::TypeFlag::GCode | Wildcards::TypeFlag::BinaryGCode, Wildcards::TypeFlag::GCode);
    } else if (tech == Technology::Sla) {
        if (extension == ".sl1" || extension == ".SL1") {
            return Wildcards::generate_wildcards(Wildcards::TypeFlag::Sl1);  
        } else if (extension == ".sl1s" || extension == ".SL1S") {
            return Wildcards::generate_wildcards(Wildcards::TypeFlag::Sl1S);  
        }
        return Wildcards::generate_wildcards(Wildcards::TypeFlag::Sl1 | Wildcards::TypeFlag::Sl1S);
    } else {
        ASSERT(false);
    }
    return {};
}

}

void show_modal_dialog(
    const Biz::ProjectInteractor& project_interactor,
    bool default_path_at_removable,
    const std::function<void(bool result, const std::vector<boost::filesystem::path>& file_paths)>& callback,
    const std::string& wildcards_overide /*= std::string()*/
)
{
    boost::filesystem::path default_folder = project_interactor.output_dir(
        project_interactor.selected_project_id(),
        default_path_at_removable,
        AppServices::instance().app_config().get<std::string>("last_used_directory")
    );

    ExportNameData name_data;
    try {
        name_data = Biz::ExportNameParser::parse_export_name(project_interactor);
     } catch (const Slic3r::PlaceholderParserError& e) {
         SPDLOG_ERROR("Failed to parse output filename: {}", e.what());

         std::string what_short = shorten_error(e.what());
         AppServices::instance().pop_notification_center().upsert_notifcation(
             {PopNotification::PopNotificationType::Custom,
              PopNotification::PopNotificationLevel::Error,
              0s,
              PopNotification::PopNotificationLayoutHeaderText("Failed to parse output filename",what_short)},
             [](const PopNotification::PopNotificationPayload&,
                const PopNotification::PopNotificationPayload&) { return false; }
         );
         // Retrieves some filename since parsing failed.
         name_data = Biz::ExportNameParser::error_state_export_name(project_interactor);
     }

     std::string wildcards = wildcards_overide.empty() ?
         gen_wildcards(name_data.preffered_extension, name_data.technology) :
         wildcards_overide;

     std::string filename = name_data.filename;

     Biz::Platform::PlatformServices::instance().main_thread_dispatcher().dispatch_on_main_thread(
         [default_folder, filename, wildcards, callback]()
         {
             AppServices::instance().dialog_manager().show_file_dialog(
                 FileDialogType::Save,
                 "Export as",
                 default_folder,
                 filename,
                 wildcards,
                 callback
             );
         }
     );
}



} // namespace  Slic3r::App
