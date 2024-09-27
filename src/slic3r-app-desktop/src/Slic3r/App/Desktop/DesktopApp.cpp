#include "DesktopApp.hpp"
#include "MainFrame.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/Init.hpp>
#include <libslic3r/Model.hpp>

#include <Slic3r/App/Platform/PlatformServices.hpp>


#include <boost/log/trivial.hpp>

wxIMPLEMENT_APP(Slic3r::App::Desktop::DesktopApp);

namespace Slic3r::App::Desktop {
bool DesktopApp::OnInit()
{
    init_logging();
    set_log_level(4);

    init_paths();
    init_translations();
    m_workbench.load_configs();
    m_project_interactor = std::make_unique<Biz::ProjectInteractor>(m_workbench);
    m_project_interactor->new_project();

    //m_workbench.load_project("/Users/jan.bartipan/work/Models/3DBenchy.3mf");

    const bool is_dark = true;
    const bool is_sys_menu = true;
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    m_main_frame = new MainFrame(m_workbench, m_project_interactor->preset_interactor(), m_translations);
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Platform::PlatformServices::instance().set_services(&canvas, &canvas);
    
    canvas.set_render_module(&m_render_module);
    m_main_frame->Show();

#if !defined(__linux)
    // Initial repaint
    canvas.render();
#endif
    return true;
}

void DesktopApp::init_translations()
{
    m_translations.init_translations(boost::filesystem::path(localization_dir()));

    // Get the active language from PrusaSlicer.ini, or empty string if the key does not exist.
    std::string language = "";// app_config->get("translation_language");
    if (!language.empty())
        BOOST_LOG_TRIVIAL(trace) << boost::format("translation_language provided by PrusaSlicer.ini: %1%") % language;

    if (!m_translations.set_best_translation_for_language(language)) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n" + WX::format_wxstr(("You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"),
            "\"locale-gen\"", "\"dpkg-reconfigure locales\"");
#endif
        message += "\n\nApplication will close.";
        wxMessageBox(message, "PrusaSlicer - Switching language failed", wxOK | wxICON_ERROR);

        std::exit(EXIT_FAILURE);
    }
    else if (!language.empty() && language != m_translations.active_language()) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
        message += "\n\n" + WX::format_wxstr(m_translations.is_alternative_language() ?
                                             "Application is started in alternative language %1%." :
                                             "Application is started in system language %1%.", m_translations.active_language());
        wxMessageBox(message, "PrusaSlicer - Switching language", wxOK | wxICON_WARNING);
    }

    // set language for Im_gui
    // m_imgui->sel_language(m_translations.active_language())
}

}
