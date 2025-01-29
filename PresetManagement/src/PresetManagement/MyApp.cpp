#include "MyApp.hpp"



#include "../Utils/Utils.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/nowide/convert.hpp>

#include <boost/log/trivial.hpp>
#include <shlobj.h>

#ifdef __WXMSW__ // Windows-specific headers for console
#include <windows.h>
#include <cstdio>
#endif

namespace {
static std::string GetDataDir()
{
    HRESULT hr = E_FAIL;

    std::wstring buffer;
    buffer.resize(MAX_PATH);

    hr = ::SHGetFolderPathW
    (
        NULL,               // parent window, not used
        CSIDL_APPDATA,
        NULL,               // access token (current user)
        SHGFP_TYPE_CURRENT, // current path, not just default value
        (LPWSTR)buffer.data()
    );

    if (hr == E_FAIL)
    {
        // directory doesn't exist, maybe we can get its default value?
        hr = ::SHGetFolderPathW
        (
            NULL,
            CSIDL_APPDATA,
            NULL,
            SHGFP_TYPE_DEFAULT,
            (LPWSTR)buffer.data()
        );
    }

    for (int i=0; i< MAX_PATH; i++)
        if (buffer.data()[i] == '\0') {
            buffer.resize(i);
            break;
        }

    return  boost::nowide::narrow(buffer);
}
}

bool MyApp::OnInit() {

#ifdef __WXMSW__
    // Allocate a console window on Windows
    if (AllocConsole()) {
        freopen("CONOUT$", "w", stdout); // Redirect stdout to console
        freopen("CONOUT$", "w", stderr); // Redirect stderr to console
        std::cout.clear();
        std::cerr.clear();
    }
#endif
    //Slic3r::set_logging_level(1);

    // resources dir
    boost::filesystem::path path_to_binary = boost::dll::program_location();
    boost::filesystem::path path_resources = path_to_binary.parent_path() / "resources";
    Slic3r::set_resources_dir(path_resources.string());
    // Verify resources path
    const wxString resources_dir = (Slic3r::resources_dir());
    wxCHECK_MSG(wxDirExists(resources_dir), false, wxString::Format("Resources path does not exist or is not a directory: %s", resources_dir));

    // datadir
    const std::string config_dir = GetDataDir();
    std::string data_dir = (boost::filesystem::path(config_dir) / "PresetManagement").make_preferred().string();
    Slic3r::set_data_dir(data_dir);
        
    if (!boost::filesystem::exists(data_dir)) {
        boost::filesystem::create_directory(data_dir);
    }
    boost::filesystem::path data_dir_path(data_dir);
    std::initializer_list<boost::filesystem::path> sub_datadirs = {
        
        // cache wont be in datadir any more, instead in system temp dir where will be left behind after use and deleted by system
        // wxStandardPaths::Get().GetTempDir()
        //data_dir + "/cache", // For download purposes, each pid should have its own sub
        
        data_dir_path / "update_sync", // Data prepared for installation, all that config wizard needs. (also pid subs?)
        data_dir_path / "shared_runtime", // all data needed for run of slicer, shared among all instances, App config, archive repo manifest, 
        data_dir_path / "snapshots",
        data_dir_path / "profiles", // subs are local or userid
        data_dir_path / "profiles" / "local", // All profiles that slicer reads on startup, vendor profiles gets here only from wizard, user profiles by user creation
        data_dir_path / "profiles" / "local" / "vendor",
        data_dir_path / "profiles" / "local" / "shapes",
        data_dir_path / "profiles" / "local" / "print",
        data_dir_path / "profiles" / "local" / "filament",
        data_dir_path / "profiles" / "local" / "sla_print",
        data_dir_path / "profiles" / "local" / "sla_material",
        data_dir_path / "profiles" / "local" / "printer",
        data_dir_path / "profiles" / "local" / "physical_printer"
    };

    for (const boost::filesystem::path& sub : sub_datadirs) {
        if (!boost::filesystem::exists(sub)) {
            boost::filesystem::create_directory(sub);
        }
    }

    // create after data dir and resources dir are set
    m_preset_updater_wrapper = std::make_unique<PresetManagement::PresetUpdaterWrapper>();
     
    // Call of check_forced_reconfigurations with simulated wait (check_forced_reconfigurations is on worker thread)
    
    std::atomic_bool error_triggered = false;
    std::atomic_bool result_triggered = false;
    std::string error_msg;
    //ReconfigurationsList list;
    auto error_fn = [&](std::string msg) {
        error_triggered = true; 
        error_msg = msg;
    };
    auto reconf_fn = [&](const PresetManagement::ReconfigurationsList& res) -> bool{
        //list = res;
        return true;
    };
    auto result_fn = [&](){
        result_triggered = true; 
    };
    m_preset_updater_wrapper->check_forced_reconfigurations(error_fn, reconf_fn, result_fn);
    std::thread wait_thread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (error_triggered || result_triggered) { return; }
        }
    });
    wait_thread.join(); 
    assert(!error_triggered);

    CallAfter([this] {
        std::atomic_bool error_triggered = false;
        std::atomic_bool result_triggered = false;
        std::string error_msg;
        auto error_fn = [&](std::string msg) {
            error_triggered = true; 
            error_msg = msg;
        };
        auto reconf_fn = [&](const PresetManagement::ReconfigurationsList& res) -> bool{
            //list = res;
            return true;
        };
        auto result_fn = [&](){
            result_triggered = true; 
        };
        m_preset_updater_wrapper->build_update_sync_and_reconfiguration_check(error_fn, reconf_fn, result_fn);
    });

    // Create the main application window (MyFrame).
    frame = new MyFrame("wxWidgets Application");

    // Bind event handler
    Bind(wxEVT_MENU, &MyApp::OnMenuClick, this, frame->menuItem->GetId());

    frame->Show(true); // Show the main frame.
    return true;       // Start the event loop.
}

void MyApp::OnMenuClick(wxCommandEvent& event) 
{
    std::atomic_bool error_triggered = false;
    std::atomic_bool result_triggered = false;
    std::string error_msg;
    auto error_fn = [&](std::string msg) {
        error_triggered = true; 
        error_msg = msg;
    };
    auto reconf_fn = [&](const PresetManagement::ReconfigurationsList& res) -> bool{
        //list = res;
        return true;
    };
    auto result_fn = [&](){
        result_triggered = true; 
    };
    m_preset_updater_wrapper->build_update_sync_and_reconfiguration_check(error_fn, reconf_fn, result_fn);
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)) {
    // Create menu bar
        wxMenuBar* menuBar = new wxMenuBar;
        wxMenu* menuFile = new wxMenu;
        
        // Add menu item
        menuItem = new wxMenuItem(menuFile, wxID_ANY, "Update Check");
        menuFile->Append(menuItem);
        
        menuBar->Append(menuFile, "File");
        SetMenuBar(menuBar);
        
        
}

