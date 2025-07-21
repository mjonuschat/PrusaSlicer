#include <catch2/catch_session.hpp>
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/SecretStoreDummy.hpp"


using Slic3r::App::Platform::StdMainThreadDispatcher;
using Slic3r::Biz::SecretStoreDummy;

int main( int argc, char* argv[] ) {
    using Slic3r::Biz::Platform::PlatformServices;

    PlatformServices::instance().set_main_thread_dispatcher(
        std::make_unique<StdMainThreadDispatcher>()
    );

    std::unique_ptr<SecretStoreDummy> store_dummy{std::make_unique<SecretStoreDummy>()};
    PlatformServices::instance().set_secret_store(std::move(store_dummy));

    const int result = Catch::Session().run( argc, argv );

    return result;
}
