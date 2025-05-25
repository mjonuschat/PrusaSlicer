#include "DesktopApp.hpp"
#include "MainFrame.hpp"
#include "AppInstanceCheck.hpp"
#include "SecretStoreFactory.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/App/Platform/WX/WXMainThreadDispatcher.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/format.hpp>
#include "Slic3r/App/WX/StringConversions.hpp"
#include <Slic3r/App/Init.hpp>
#include <Slic3r/App/Localization.hpp>
#include <Slic3r/App/ResourceResolver.hpp>
#include <libslic3r/Model.hpp>

#include <Slic3r/App/Render/TextureManager.hpp>

#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/Biz/Platform/Termination.hpp>

#include "Slic3r/App/WX/DialogManager.hpp"

#include <boost/log/trivial.hpp>

wxIMPLEMENT_APP_NO_MAIN(Slic3r::App::Desktop::DesktopApp);


namespace Slic3r::App::Desktop {

namespace {
#ifdef WIN32
void register_win32_device_notification_event()
{
    wxWindow::MSWRegisterMessageHandler(WM_COPYDATA, [](wxWindow* win, WXUINT /* nMsg */, WXWPARAM wParam, WXLPARAM lParam) {
        auto* app_instance = dynamic_cast<Slic3r::App::Desktop::DesktopApp*>(wxTheApp); 
		COPYDATASTRUCT* copy_data_structure = { 0 };
		copy_data_structure = (COPYDATASTRUCT*)lParam;
		if (copy_data_structure->dwData == 1) {
			LPCWSTR arguments = (LPCWSTR)copy_data_structure->lpData;
            std::string args = WX::into_u8(arguments);
            SPDLOG_INFO("MSG {}", args);
            app_instance->handle_app_instance_message(args);
		}
		return true;
	});
}
#endif // WIN32
}

int run(const Slic3r::App::InitParams& init_params)
{
    init_paths(); // instance_check needs data_dir()
    bool single_instance_app_config = false; // TODO: read app config for this value
    if (AppInstance::instance_check(init_params, single_instance_app_config)) { 
        return 1;
    }
    Render::TextureManager::set_resource_resolver(std::make_unique<ResourceResolver>(resources_dir()));
    auto* app = new Slic3r::App::Desktop::DesktopApp();
    Slic3r::App::Desktop::DesktopApp::SetInstance(app);
    int argc = init_params.argc;
    char** argv = init_params.argv;
    app->set_init_params(init_params); // this is called before OnInit.
    return wxEntry(argc, argv);
}

bool DesktopApp::OnInit()
{
    init_logging();
    set_log_level(4);

    
    init_translations();
    m_workbench.load_configs();

    using Platform::WX::WXMainThreadDispatcher;
    using Biz::Platform::PlatformServices;

    PlatformServices::instance().set_main_thread_dispatcher(
        std::make_unique<WXMainThreadDispatcher>()
    );

    PlatformServices::instance().set_secret_store(SecretStoreFactory::create_secret_store());

    m_project_interactor = std::make_unique<Biz::ProjectInteractor>(
        m_workbench,
        PlatformServices::instance().main_thread_dispatcher()
    );
    m_plater_module =
        std::make_unique<Plater::PlaterRenderModule>(m_workbench, *m_project_interactor);
    m_preview_module =
        std::make_unique<Preview::PreviewRenderModule>(m_workbench, *m_project_interactor);
    DialogManagerProvider::instance().set_dialog_manager_implementation(std::make_unique<WX::DialogManager>());

    const bool is_dark = true;
    const bool is_sys_menu = true;
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    m_project_interactor->new_project();

    m_main_frame = new MainFrame(m_workbench, *m_project_interactor);
    m_project_interactor->init_app_instance_message_handler(m_main_frame->GetHandle());
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Biz::Platform::PlatformServices::instance().set_render_request_handler(&canvas);
    m_main_frame->update_canvas_ui_settings();

    m_navigator.on_init(*m_plater_module, *m_preview_module, canvas);

    canvas.set_render_module(m_plater_module.get());

    m_main_frame->Show();

#if !defined(__linux)
    // Initial repaint
    canvas.render();
#endif

    // force bed selection to update bed changed listeners
    m_project_interactor->scene_interactor().select_first_bed_instance();

    // temp solution because of ScenePresenter is created in canvas.render()
 //   m_project_interactor->load_project("C:\\PS_3\\Test_ObjectList.3mf");
    
#ifdef WIN32
    register_win32_device_notification_event();
#endif // WIN32

    return true;
}

bool DesktopApp::OnExceptionInMainLoop() {
    try {
        throw;
    } catch (...){
        // TODO: currently there is no handling!
        throw;
    }
}

void DesktopApp::OnUnhandledException() {
    try {
        throw;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("closing after unrecoverable exception: '{}'", e.what());
    } catch ( ... ) {
        SPDLOG_ERROR("closing after unrecoverable unknown exception");
    }
    Biz::Platform::close();
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

void DesktopApp::handle_app_instance_message(const std::string& message)
{
    m_project_interactor->handle_app_instance_message(message);
}

}
