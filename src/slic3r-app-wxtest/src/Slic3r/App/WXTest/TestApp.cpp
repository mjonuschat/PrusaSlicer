#include "TestApp.hpp"
#include "MainFrame.hpp"
#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <libslic3r/Utils.hpp>

wxIMPLEMENT_APP(Slic3r::App::WXTest::TestApp);

namespace Slic3r::App::WXTest {
bool TestApp::OnInit()
{
    // See Invoking prusa-slicer from $PATH environment variable crashes #5542
    // boost::filesystem::path path_to_binary = boost::filesystem::system_complete(argv[0]);
    boost::filesystem::path path_to_binary = boost::dll::program_location();

    // Path from the Slic3r binary to its resources.
#ifdef __APPLE__
    // The application is packed in the .dmg archive as 'Slic3r.app/Contents/MacOS/Slic3r'
    // The resources are packed to 'Slic3r.app/Contents/Resources'
    boost::filesystem::path path_resources = boost::filesystem::canonical(path_to_binary).parent_path() / "../Resources";
#elif defined _WIN32
    // The application is packed in the .zip archive in the root,
    // The resources are packed to 'resources'
    // Path from Slic3r binary to resources:
    boost::filesystem::path path_resources = path_to_binary.parent_path() / "resources";
#elif defined SLIC3R_FHS
    // The application is packaged according to the Linux Filesystem Hierarchy Standard
    // Resources are set to the 'Architecture-independent (shared) data', typically /usr/share or /usr/local/share
    boost::filesystem::path path_resources = SLIC3R_FHS_RESOURCES;
#else
    // The application is packed in the .tar.bz archive (or in AppImage) as 'bin/slic3r',
    // The resources are packed to 'resources'
    // Path from Slic3r binary to resources:
    boost::filesystem::path path_resources = boost::filesystem::canonical(path_to_binary).parent_path() / "../resources";
#endif

    set_resources_dir(path_resources.string());
    set_var_dir((path_resources / "icons").string());
    set_local_dir((path_resources / "localization").string());
    set_sys_shapes_dir((path_resources / "shapes").string());
    set_custom_gcodes_dir((path_resources / "custom_gcodes").string());

    m_main_frame = new MainFrame();
    Platform::WX::WXRenderCanvas& canvas = m_main_frame->get_render_canvas();
    Platform::PlatformServices::instance().set_services(&canvas, &canvas);

    m_render_module = std::make_unique<TestRenderModule>();
    canvas.set_render_module(m_render_module.get());
    m_main_frame->Show();

#ifndef __linux__
    // Initial repaint
    canvas.render();
#endif

    return true;
}

}
