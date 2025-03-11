#include "DesktopApp.hpp"
#include "MainFrame.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/App/Platform/WX/WXMainThreadDispatcher.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/App/Localization.hpp>
#include <libslic3r/Model.hpp>

#include <Slic3r/Biz/Platform/PlatformServices.hpp>


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

    using Platform::WX::WXMainThreadDispatcher;
    using Biz::Platform::PlatformServices;

    PlatformServices::instance().set_main_thread_dispatcher(
        std::make_unique<WXMainThreadDispatcher>()
    );

    m_project_interactor = std::make_unique<Biz::ProjectInteractor>(
        m_workbench,
        PlatformServices::instance().main_thread_dispatcher()
    );
    m_plater_module =
      std::make_unique<Plater::PlaterRenderModule>(m_workbench, *m_project_interactor);
    m_preview_module =
      std::make_unique<Preview::PreviewRenderModule>(m_workbench, *m_project_interactor);

    const bool is_dark = true;
    const bool is_sys_menu = true;
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    m_project_interactor->new_project();

    m_main_frame = new MainFrame(m_workbench, m_project_interactor->preset_interactor());
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Biz::Platform::PlatformServices::instance().set_render_request_handler(&canvas);
    m_main_frame->update_canvas_ui_settings();

    // >>> replace m_plater_module with m_preview_module in the following line to test libvgcode wrapper
    canvas.set_render_module(m_preview_module.get());
    m_project_interactor->fdm_result_cache().add_listener<Biz::IFDMResultCacheChangedListener>(
        m_preview_module.get()
    );

    m_main_frame->Show();

#if !defined(__linux)
    // Initial repaint
    canvas.render();
#endif

    // temp solution because of ScenePresenter is created in canvas.render()
 //   m_project_interactor->load_project("C:\\PS_3\\Test_ObjectList.3mf");

    return true;
}

void DesktopApp::init_translations()
{
    // Get the active language from PrusaSlicer.ini, or empty string if the key does not exist.
    std::string language = "";// app_config->get("translation_language");
    if (!language.empty())
        BOOST_LOG_TRIVIAL(trace) << boost::format("translation_language provided by PrusaSlicer.ini: %1%") % language;

    if (!localization().set_language(language)) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n" + WX::format_wxstr(("You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"),
            "\"locale-gen\"", "\"dpkg-reconfigure locales\"");
#endif
        message += WX::from_u8("\n\nApplication will close.");
        wxMessageBox(message, WX::from_u8("PrusaSlicer - Switching language failed"), wxOK | wxICON_ERROR);

        std::exit(EXIT_FAILURE);
    }
    else if (!language.empty() && language != localization().active_language()) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
        message += WX::from_u8("\n\n") + WX::format_wxstr(localization().is_alternative_language() ?
                                             "Application is started in alternative language %1%." :
                                             "Application is started in system language %1%.", localization().active_language());
        wxMessageBox(message, WX::from_u8("PrusaSlicer - Switching language"), wxOK | wxICON_WARNING);
    }
}

}
