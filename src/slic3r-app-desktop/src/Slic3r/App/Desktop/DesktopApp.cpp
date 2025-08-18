#include "DesktopApp.hpp"
#include "MainFrame.hpp"
#include "SplashScreen.hpp"
#include "AppInstanceCheck.hpp"
#include "SecretStoreFactory.hpp"
#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/App/Platform/WX/WXMainThreadDispatcher.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/WX/I18N.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/App/Localization.hpp>
#include <Slic3r/App/ResourceResolver.hpp>
#include <Slic3r/App/SharedThumbnailImageGenerator.hpp>
#include <Slic3r/App/ThumbnailStore.hpp>
#include <Slic3r/App/ThumbnailStoreUpdater.hpp>
#include <Slic3r/App/PopNotification/PopNotificationCenter.hpp>
#include <Slic3r/App/AppServices.hpp>
#include <Slic3r/App/PopNotification/PopNotificationFactory.hpp>
#include <Slic3r/App/WX/FileExplorerHandler.hpp>

#include "Slic3r/Directories.hpp"
#include <Slic3r/App/Render/TextureManager.hpp>

#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/Biz/Platform/JobManager/JobManager.hpp>
#include <Slic3r/Biz/Platform/Termination.hpp>
#include <Slic3r/Biz/Slicing/SlicingInteractor.hpp>
#include <Slic3r/Biz/PrintHost/PrintHostInteractor.hpp>
#include "Slic3r/Biz/UserAccount/UserAccountInteractor.hpp"

#include "Slic3r/App/WX/DialogManager.hpp"

#include "libslic3r/Utils.hpp"

// TODO: replace with spdlog
#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

#ifdef WIN32
#include <dbt.h>
#include <shlobj.h>
static GUID GUID_DEVINTERFACE_HID = {0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};
#endif // WIN32

wxIMPLEMENT_APP_NO_MAIN(Slic3r::App::Desktop::DesktopApp);

namespace fs = boost::filesystem;

namespace Slic3r::App::Desktop {

namespace {

#ifdef WIN32

void register_win32_device_notification_event()
{
    wxWindow::MSWRegisterMessageHandler(
        WM_COPYDATA,
        [](wxWindow* win, WXUINT /* nMsg */, WXWPARAM wParam, WXLPARAM lParam)
        {
            auto* app_instance = dynamic_cast<Slic3r::App::Desktop::DesktopApp*>(wxTheApp);
            COPYDATASTRUCT* copy_data_structure = {0};
            copy_data_structure                 = (COPYDATASTRUCT*) lParam;
            if (copy_data_structure->dwData == 1) {
                LPCWSTR arguments = (LPCWSTR) copy_data_structure->lpData;
                std::string args  = WX::into_u8(arguments);
                SPDLOG_INFO("MSG {}", args);
                app_instance->handle_app_instance_message(args);
            }
            return true;
        }
    );

    wxWindow::MSWRegisterMessageHandler(
        WM_DEVICECHANGE,
        [](wxWindow* win, WXUINT /* nMsg */, WXWPARAM wParam, WXLPARAM lParam)
        {
            auto* app_instance      = dynamic_cast<Slic3r::App::Desktop::DesktopApp*>(wxTheApp);
            PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR) lParam;
            switch (wParam) {
            case DBT_DEVICEARRIVAL:
                if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                    app_instance->handle_volumes_changed_event();
                } else if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    PDEV_BROADCAST_DEVICEINTERFACE lpdbi = (PDEV_BROADCAST_DEVICEINTERFACE) lpdb;
                    if (lpdbi->dbcc_classguid == GUID_DEVINTERFACE_HID) {
                        app_instance->handle_HID_device_attached_event(WX::into_u8(lpdbi->dbcc_name));
                    }
                }
                break;
            case DBT_DEVICEREMOVECOMPLETE:
                if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                    app_instance->handle_volumes_changed_event();
                } else if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    PDEV_BROADCAST_DEVICEINTERFACE lpdbi = (PDEV_BROADCAST_DEVICEINTERFACE) lpdb;
                    if (lpdbi->dbcc_classguid == GUID_DEVINTERFACE_HID) {
                        app_instance->handle_HID_device_detached_event(WX::into_u8(lpdbi->dbcc_name));
                    }
                }
                break;
            default:
                break;
            }
            return true;
        }
    );

    wxWindow::MSWRegisterMessageHandler(
        MainFrame::WM_USER_MEDIACHANGED,
        [](wxWindow* win, WXUINT /* nMsg */, WXWPARAM wParam, WXLPARAM lParam)
        {
            auto* app_instance = dynamic_cast<Slic3r::App::Desktop::DesktopApp*>(wxTheApp);
            wchar_t sPath[MAX_PATH];
            if (lParam == SHCNE_MEDIAINSERTED || lParam == SHCNE_MEDIAREMOVED) {
                struct _ITEMIDLIST* pidl = *reinterpret_cast<struct _ITEMIDLIST**>(wParam);
                if (!SHGetPathFromIDList(pidl, sPath)) {
                    return false;
                }
            }
            switch (lParam) {
            case SHCNE_MEDIAINSERTED: {
                app_instance->handle_volumes_changed_event();
                break;
            }
            case SHCNE_MEDIAREMOVED: {
                app_instance->handle_volumes_changed_event();
                break;
            }
            default:
                break;
            }
            return true;
        }
    );
}
#endif // WIN32
} // namespace

int run(const Slic3r::App::InitParams& init_params)
{
    init_paths(); // instance_check needs data_dir()
    bool single_instance_app_config = false; // TODO: read app config for this value
    if (AppInstance::instance_check(init_params, single_instance_app_config)) {
        return 1;
    }
    Render::TextureManager::set_resource_resolver(std::make_unique<ResourceResolver>(Biz::resources_dir()));
    auto* app = new Slic3r::App::Desktop::DesktopApp();
    Slic3r::App::Desktop::DesktopApp::SetInstance(app);
    int argc    = init_params.argc;
    char** argv = init_params.argv;
    app->set_init_params(init_params); // this is called before OnInit.
    return wxEntry(argc, argv);
}

bool DesktopApp::OnInit()
{
    // Set initialization of image handlers before any UI actions - See GH issue #7469
    wxInitAllImageHandlers();

    init_logging();
    set_log_level(4);

    init_translations();

    const bool is_dark             = true;
    const bool is_sys_menu         = true;
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    bool is_editor     = true; // is_editor();
    SplashScreen* scrn = nullptr;
    if (1 /*app_config->get_bool("show_splash_screen")*/) {
        // Detect position (display) to show the splash screen
        // Now this position is equal to the mainframe position
        wxPoint splashscreen_pos      = wxDefaultPosition;
        bool default_splashscreen_pos = true;
        /* ysFIXME uncomment, when we start to process app_config
                if (app_config->has("window_mainframe") && app_config->get_bool("restore_win_position")) {
                    auto metrics = WindowMetrics::deserialize(app_config->get("window_mainframe"));
                    default_splashscreen_pos = metrics == boost::none;
                    if (!default_splashscreen_pos)
                        splashscreen_pos = metrics->get_rect().GetPosition();
                }

                if (!default_splashscreen_pos) {
                    // workaround for crash related to the positioning of the window on secondary monitor
                    get_app_config()->set("restore_win_position", "crashed_at_splashscreen_pos");
                    get_app_config()->save();
                }
        */
        // create splash screen with updated bmp
        scrn = new SplashScreen(is_editor, splashscreen_pos);

/* ysFIXME uncomment, when we start to process app_config
        if (!default_splashscreen_pos) {
            // revert "restore_win_position" value if application wasn't crashed
            get_app_config()->set("restore_win_position", "1");
        }
*/
#ifndef __linux__
        wxYield();
#endif
        scrn->SetText(L("Loading configurations") + "...");
    }

    using Biz::Platform::PlatformServices;
    using Biz::Platform::JobManager::JobManager;
    using Platform::WX::WXMainThreadDispatcher;

    auto& platform_services{PlatformServices::instance()};
    auto& app_services{AppServices::instance()};

    platform_services.set_main_thread_dispatcher(std::make_unique<WXMainThreadDispatcher>());

    platform_services.set_secret_store(SecretStoreFactory::create_secret_store());

    platform_services.set_job_manager(std::make_unique<JobManager>(platform_services.main_thread_dispatcher()));

    std::shared_ptr<Plater::ThumbnailImageGenerator> thumbnail_image_generator{std::make_shared<Plater::ThumbnailImageGenerator>()};

    m_project_interactor = std::make_unique<Biz::ProjectInteractor>(m_workbench, platform_services.main_thread_dispatcher(), *thumbnail_image_generator);

    std::shared_ptr<App::ThumbnailStore> thumbnail_store = std::make_shared<App::ThumbnailStore>(*m_project_interactor);

    std::shared_ptr<App::ThumbnailStoreUpdater> thumbnail_store_updater = std::make_shared<App::ThumbnailStoreUpdater>(*thumbnail_image_generator, thumbnail_store);

    auto& preset_interactor = m_project_interactor->preset_interactor();

    // load new presets
    fs::path preset_bundle_dir = fs::path{Slic3r::resources_dir()} / "presets";
    fs::path config_dir        = fs::path{data_dir()} / "configs";
    preset_interactor.load_preset_bundle(preset_bundle_dir.string(), config_dir.string());

    std::shared_ptr<App::SharedThumbnailImageGenerator> shared_thumbnail_image_generator = std::make_shared<App::SharedThumbnailImageGenerator>();
    app_services.set_dialog_manager(std::make_unique<WX::DialogManager>());
    app_services.set_pop_notification_center(
        std::make_unique<PopNotification::PopNotificationCenter>(m_project_interactor->removable_drive_service())
    );
    app_services.set_file_explorer_handler(std::make_unique<WX::FileExplorerHandler>());
    platform_services.job_manager().add_listener<Biz::Platform::JobManager::IJobManagerStatusChangedListener>(
        &app_services.pop_notification_center()
    );
    m_project_interactor->user_account_interactor()
        .add_listener<Biz::UserAccount::IUserAccountListener>(&app_services.pop_notification_center());
    if (scrn && is_editor)
        scrn->SetText(L("Preparing Plater") + "...");

    m_plater_module = std::make_unique<Plater::PlaterRenderModule>(
        m_workbench,
        *m_project_interactor,
        thumbnail_store,
        thumbnail_store_updater,
        thumbnail_image_generator
    );

    if (scrn && is_editor)
        scrn->SetText(L("Preparing Preview") + "...");

    m_preview_module = std::make_unique<Preview::PreviewRenderModule>(
        m_workbench,
        *m_project_interactor,
        thumbnail_store,
        thumbnail_store_updater,
        thumbnail_image_generator
    );

    const bool is_dark     = true;
    const bool is_sys_menu = true;
    m_project_interactor->slicing_interactor().add_listener<Biz::Slicing::IStatusListener>(
        &app_services.pop_notification_center()
    );
    m_project_interactor->print_host_interactor().add_print_host_listener(&app_services.pop_notification_center());
    m_project_interactor->removable_drive_service().add_status_listener(&app_services.pop_notification_center());
    WX::WidgetsConfig* wdts_config = WX::WidgetsConfig::instance(is_dark, is_sys_menu);

    m_project_interactor->new_project();

    m_main_frame = new MainFrame(m_workbench, *m_project_interactor);
    m_project_interactor->init_app_instance_message_handler(m_main_frame->GetHandle());
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    m_gl_context                         = canvas.release_context();
    platform_services.set_render_request_handler(&canvas);
    m_main_frame->update_canvas_ui_settings();

    m_navigator.on_init(*m_plater_module, *m_preview_module, canvas, m_project_interactor.get());

    canvas.set_render_module(m_plater_module.get());

    m_main_frame->Show();

    m_preset_updater_ui = std::make_unique<PresetUpdaterUI>(m_project_interactor->preset_updater_interactor());

#ifdef WIN32
    m_main_frame->register_win32_callbacks();
#endif

    platform_services.instance()
        .job_manager()
        .create_job(
            "countdown",
            [](Biz::JThread::StopToken stop_token, Biz::Platform::IMainThreadDispatcher& dis, Biz::Platform::JobManager::ProgressTracker progress)
            {
                for (size_t i = 0; i < 100; i++) {
                    std::this_thread::sleep_for(1'000ms);
                    Slic3r::Domain::Percentage p;
                    p.value = (double) i / 100.;
                    progress.set(p);
                    AppServices::instance().pop_notification_center().add_notification(
                        PopNotification::PopNotificationFactory::create_custom(PopNotification::PopNotificationLevel::Important, 0, "test " + std::to_string(i))
                    );
                }
            }
        )
        .start();

#if !defined(__linux)
    // Initial repaint
    canvas.render();
#endif

#ifdef WIN32
    register_win32_device_notification_event();
#endif // WIN32

    return true;
}

bool DesktopApp::OnExceptionInMainLoop()
{
    try {
        throw;
    } catch (...) {
        // TODO: currently there is no handling!
        throw;
    }
}

void DesktopApp::OnUnhandledException()
{
    try {
        throw;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("closing after unrecoverable exception: '{}'", e.what());
    } catch (...) {
        SPDLOG_ERROR("closing after unrecoverable unknown exception");
    }
    Biz::Platform::close();
}

void DesktopApp::init_translations()
{
    // Get the active language from PrusaSlicer.ini, or empty string if the key does not exist.
    std::string language = ""; // app_config->get("translation_language");
    if (!language.empty())
        BOOST_LOG_TRIVIAL(trace) << boost::format("translation_language provided by PrusaSlicer.ini: %1%") % language;

    if (!localization().set_language(language)) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n" + WX::format_wxstr(("You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"), "\"locale-gen\"", "\"dpkg-reconfigure locales\"");
#endif
        message += WX::from_u8("\n\nApplication will close.");
        wxMessageBox(message, WX::from_u8("PrusaSlicer - Switching language failed"), wxOK | wxICON_ERROR);

        std::exit(EXIT_FAILURE);
    } else if (!language.empty() && language != localization().active_language()) {
        // Loading the language dictionary failed.
        wxString message = WX::format_wxstr("Switching PrusaSlicer to language %1% failed.", language);
        message +=
            WX::from_u8("\n\n")
            + WX::format_wxstr(
                localization().is_alternative_language() ? "Application is started in alternative language %1%." : "Application is started in system language %1%.",
                localization().active_language()
            );
        wxMessageBox(message, WX::from_u8("PrusaSlicer - Switching language"), wxOK | wxICON_WARNING);
    }
}

void DesktopApp::handle_app_instance_message(const std::string& message)
{
    m_project_interactor->handle_app_instance_message(message);
}

void DesktopApp::handle_HID_device_detached_event(const std::string& message)
{
    // TODO: Add 3d mouse control
}

void DesktopApp::handle_HID_device_attached_event(const std::string& message)
{
    // TODO: Add 3d mouse control
}

void DesktopApp::handle_volumes_changed_event()
{
    m_project_interactor->handle_volumes_changed_event();
}

#ifdef __APPLE__
void DesktopApp::MacOpenURL(const wxString& url)
{
    std::string narrow_url = WX::into_u8(url);
    if (boost::starts_with(narrow_url, "prusaslicer://login")) {
        m_project_interactor->user_account_interactor().on_log_in_code_response(narrow_url);
    } else {
        SPDLOG_ERROR("MacOpenURL recieved improper URL: {}", narrow_url);
    }
}
#endif /* __APPLE__ */

} // namespace Slic3r::App::Desktop
