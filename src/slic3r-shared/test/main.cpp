#include <catch2/catch_session.hpp>
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"


using Slic3r::App::Platform::StdMainThreadDispatcher;

int main( int argc, char* argv[] ) {
    using Slic3r::Biz::Platform::PlatformServices;

    PlatformServices::instance().set_main_thread_dispatcher(
        std::make_unique<StdMainThreadDispatcher>()
    );
    const int result = Catch::Session().run( argc, argv );

    return result;
}
