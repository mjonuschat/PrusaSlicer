#include "Slic3r/App/Browser/BrowserLogicConnectSelect.hpp"

#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Browser {


BrowserLogicConnectSelect::BrowserLogicConnectSelect(Biz::ProjectInteractor& project_interactor)
    : AbstractBrowserLogic(Biz::Network::ServiceConfig::instance().connect_select_printer_url(), {"_prusaSlicer"}, "connect_loading", "connect_error")
    , AbstractConnectRequestHandler(project_interactor)
{
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_script_message_webview_event(const std::string& message) 
{
     return handle_message(message);
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_show_webview_event(bool show)
{
    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_connect_action_select_printer(const std::string& message_data)
{
    ASSERT(false, "SELECT_PRINTER request is not defined for Connect Select");
    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_connect_action_print(const std::string& message_data)
{
    m_project_interactor.do_upload_connect(m_project_interactor.selected_bed_slicing_id(), message_data);
    return {{BrowserLogicCommandType::EndModalOK, {}}};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_connect_action_webapp_ready(const std::string& message_data)
{
    std::string placeholder_script = "window._prusaConnect_v2.requestCompatiblePrinter({\"printerUuid\": \"\"})";
    return {{BrowserLogicCommandType::RunScript, placeholder_script}};
    /*
    if (Preset::printer_technology(wxGetApp().preset_bundle->printers.get_selected_preset().config) == ptFFF) {
        request_compatible_printers_FFF();
    } else {
        request_compatible_printers_SLA();
    }
    */
    
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::request_compatible_printers_FFF()
{
    // TODO
    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::request_compatible_printers_SLA()
{
    // TODO
    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_webview_reload_event(const std::string& message_data)
{
    return {{BrowserLogicCommandType::LoadURL, m_url}};
}

std::vector<BrowserLogicCommand> BrowserLogicConnectSelect::on_connect_action_close_dialog(const std::string& message_data)
{
    return {{BrowserLogicCommandType::EndModalOK, {}}};
}


} // namespace Slic3r::App::Browser 