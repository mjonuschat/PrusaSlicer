#include "Slic3r/Biz/Network/MockHttp.hpp"
#include "Slic3r/Biz/Network/MockHttpFactory.hpp"

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/SecretStoreDummy.hpp"
#include "Slic3r/TestUtils/AppInstanceMessageHandlerScope.hpp"
#include "Slic3r/TestUtils/JobManagerScope.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Semver.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/fstream.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace Slic3r::Biz;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;

namespace {

const std::string k_repo_name     = "test_repo";
const std::string k_vendor_name   = "TestVendor";
const std::string k_second_vendor = "SecondVendor";
const std::string k_server_addr   = "http://localhost:8000/";

using ResponseMutator = std::function<void(const std::string& url, std::string& body, unsigned& status)>;

void copy_dir_content_local(const fs::path& from, const fs::path& to)
{
    ASSERT(fs::exists(from) && fs::is_directory(from));
    if (!fs::exists(to)) {
        fs::create_directories(to);
    }
    boost::system::error_code ec;
    for (fs::recursive_directory_iterator it(from); it != fs::recursive_directory_iterator();
         it.increment(ec)) {
        ASSERT(!ec);
        const fs::path relative_path = it->path().lexically_relative(from);
        const fs::path dest_path     = to / relative_path;
        if (fs::is_directory(it->status())) {
            if (!fs::exists(dest_path)) {
                fs::create_directories(dest_path);
            }
        } else if (fs::is_regular_file(it->status())) {
            fs::copy_file(it->path(), dest_path, fs::copy_options::overwrite_existing);
        }
    }
}

std::string read_file(const fs::path& path)
{
    boost::nowide::ifstream file(path, std::ios::in | std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/// Captures every result the interactor dispatches so a test can assert on it afterwards.
class CaptureListener : public PresetUpdater::IPresetUpdaterResultListener
{
public:
    explicit CaptureListener(PresetUpdater::PresetUpdaterInteractor& interactor) :
        m_interactor(interactor)
    {
        m_interactor.add_listener<PresetUpdater::IPresetUpdaterResultListener>(this);
    }

    void reset()
    {
        m_has_result = false;
        m_error      = false;
        m_performed  = false;
        m_error_body.clear();
        m_reconfigurations.reset();
        m_repos.clear();
        m_warnings.clear();
    }

    bool has_result() const { return m_has_result; }
    bool got_error() const { return m_error; }
    const std::string& error_body() const { return m_error_body; }
    bool performed() const { return m_performed; }
    const std::optional<PresetUpdater::PresetUpdaterReconfigurationList>& reconfigurations() const
    {
        return m_reconfigurations;
    }
    const PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& repos() const { return m_repos; }
    const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings() const { return m_warnings; }

    void on_preset_updater_error(const std::string& body) override
    {
        m_error      = true;
        m_error_body = body;
        m_has_result = true;
    }

    void on_preset_updater_reconfigurations_list(
        const PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings,
        PresetUpdater::VerboseStyle
    ) override
    {
        m_reconfigurations = reconfigurations;
        m_warnings         = warnings;
        m_has_result       = true;
    }

    void on_preset_updater_forced_reconfigurations_list(
        const PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings
    ) override
    {
        m_reconfigurations = reconfigurations;
        m_warnings         = warnings;
        m_has_result       = true;
    }

    void on_preset_updater_reconfigurations_performed(
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings
    ) override
    {
        m_performed  = true;
        m_warnings   = warnings;
        m_has_result = true;
    }

    void on_preset_updater_status(const std::string&, int, unsigned, PresetUpdater::VerboseStyle) override {}

    void on_preset_updater_repository_info_vector(
        const PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings
    ) override
    {
        store_repos(descriptor, warnings);
    }

    void on_preset_updater_repository_selection_performed(
        const PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings
    ) override
    {
        store_repos(descriptor, warnings);
    }

private:
    void store_repos(
        const PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<PresetUpdater::PresetUpdaterWarning>& warnings
    )
    {
        m_repos.assign(descriptor.begin(), descriptor.end());
        m_warnings   = warnings;
        m_has_result = true;
    }

    PresetUpdater::PresetUpdaterInteractor& m_interactor;
    bool m_has_result{false};
    bool m_error{false};
    bool m_performed{false};
    std::string m_error_body;
    std::optional<PresetUpdater::PresetUpdaterReconfigurationList> m_reconfigurations;
    PresetUpdater::SharedPresetUpdaterRepositoryInfoVector m_repos;
    std::vector<PresetUpdater::PresetUpdaterWarning> m_warnings;
};

/// Sets up the mock-server world used by PresetUpdaterStageTests and restores the global dirs it
/// changed on teardown, so the fixture leaves no state behind.
struct Fixture
{
    fs::path resource_dir;
    fs::path data_dir;
    fs::path installed_path;
    fs::path staged_path;
    fs::path shared_runtime_path;
    fs::path resources_repo_path;
    fs::path server_runtime_path;
    fs::path resources_profile_path;
    fs::path temp_dir_path;

    std::shared_ptr<ResponseMutator> mutator_slot = std::make_shared<ResponseMutator>();

    std::string prev_resources_dir;
    std::string prev_data_dir;
    fs::path prev_temp_dir;

    Slic3r::App::Platform::StdMainThreadDispatcher dispatcher;
    Tests::AppInstanceMessageHandlerScope app_instance_message_handler_scope{dispatcher};
    Tests::JobManagerScope job_manager_scope{dispatcher};
    Slic3r::Domain::Workbench workbench;
    Slic3r::Test::MockThumbnailImageGenerator thumbnail_image_generator;
    Slic3r::Biz::ProjectInteractor project_interactor;

    Fixture() : project_interactor(workbench, dispatcher, thumbnail_image_generator)
    {
        boost::nowide::nowide_filesystem();

        prev_resources_dir = Slic3r::resources_dir();
        prev_data_dir      = Slic3r::data_dir();
        prev_temp_dir      = Slic3r::temp_dir();

        Slic3r::Biz::Network::configure_http_factory_with_mock();

        resource_dir = Tests::get_datadir();
        Slic3r::set_resources_dir(resource_dir.string());
        Slic3r::set_data_dir((resource_dir / "datadir_actions").string());
        data_dir = fs::path(Slic3r::data_dir());

        installed_path         = data_dir / "presets" / "local";
        staged_path            = data_dir / "update_sync";
        shared_runtime_path    = data_dir / "shared_runtime";
        resources_repo_path    = resource_dir / "presets" / k_repo_name;
        server_runtime_path    = resource_dir / "server" / "runtime_actions";
        resources_profile_path = resource_dir / "preset updater";
        temp_dir_path          = resource_dir / "temp_actions";

        Network::ServiceConfig::instance().set_preset_repo_url(k_server_addr);

        fs::remove_all(data_dir);
        fs::remove_all(resources_repo_path);
        fs::remove_all(server_runtime_path);
        fs::remove_all(temp_dir_path);

        fs::create_directories(installed_path);
        fs::create_directories(staged_path);
        fs::create_directories(shared_runtime_path);
        fs::create_directories(resources_repo_path);
        fs::create_directories(server_runtime_path);
        fs::create_directories(temp_dir_path);

        Slic3r::set_temp_dir(temp_dir_path);

        fs::copy_file(
            resources_profile_path / "RepositoryManifest.json",
            shared_runtime_path / "RepositoryManifest.json",
            fs::copy_options::overwrite_existing
        );

        auto slot                     = mutator_slot;
        const fs::path server_runtime = server_runtime_path;
        Network::MockHttpProvider::get().set_create_fn(
            [server_runtime, slot](
                Network::IHttp::RequestMethod method,
                const std::string& url,
                Network::IHttp::RetryFn retryfn
            ) -> std::unique_ptr<Network::MockHttp> {
                ASSERT(method == Network::IHttp::RequestMethod::Get);
                std::unique_ptr<Network::MockHttp> ret =
                    std::make_unique<Network::MockHttp>(url, retryfn);
                ret->set_perform_override_fn(
                    [url, server_runtime, slot](Network::MockHttp* http, const Network::HttpRetryOpt&) {
                        std::string aux(url);
                        if (aux.starts_with(k_server_addr)) {
                            aux.replace(0, k_server_addr.length(), "");
                        }
                        const fs::path path = server_runtime / aux;
                        boost::nowide::ifstream file(path, std::ios::in | std::ios::binary);
                        if (file.is_open()) {
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            std::string body = buffer.str();
                            unsigned status  = 200;
                            if (slot && *slot) {
                                (*slot)(url, body, status);
                            }
                            http->invoke_complete(body, status);
                        } else {
                            http->invoke_error("", "File not found: " + path.string(), 404);
                        }
                    }
                );
                ret->default_allow_timeout = NAMED_ALLOW_CALL(*ret, timeout_total_mock(trompeloeil::_));
                ret->default_allow_size_limit = NAMED_ALLOW_CALL(*ret, size_limit_mock(trompeloeil::_));
                return ret;
            }
        );

        Platform::PlatformServices::instance().set_secret_store(std::make_unique<SecretStoreDummy>());
    }

    ~Fixture()
    {
        dispatcher.close();
        fs::remove_all(resources_repo_path);
        fs::remove_all(data_dir);
        fs::remove_all(server_runtime_path);
        fs::remove_all(temp_dir_path);
        Slic3r::set_resources_dir(prev_resources_dir);
        Slic3r::set_data_dir(prev_data_dir);
        Slic3r::set_temp_dir(prev_temp_dir);
    }

    PresetUpdater::PresetUpdaterInteractor& interactor()
    {
        return project_interactor.preset_updater_interactor();
    }

    void set_mutator(ResponseMutator m) { *mutator_slot = std::move(m); }

    void put_resources(const std::string& version)
    {
        copy_dir_content_local(
            resources_profile_path / ("resource" + version) / k_repo_name,
            resources_repo_path
        );
    }

    void put_server(const std::string& version)
    {
        copy_dir_content_local(resources_profile_path / ("server" + version), server_runtime_path);
    }

    void put_installed(const std::string& version)
    {
        copy_dir_content_local(resources_profile_path / ("resource" + version), installed_path);
    }

    void put_staged(const std::string& version)
    {
        copy_dir_content_local(resources_profile_path / ("resource" + version), staged_path);
    }

    bool wait(CaptureListener& listener, std::chrono::seconds timeout = 20s)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        while (true) {
            dispatcher.dispatch_enqueued();
            if (listener.has_result()) {
                return true;
            }
            if (std::chrono::high_resolution_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(1ms);
        }
    }
};

} // namespace

TEST_CASE("PresetUpdater performs a new-vendor install", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    REQUIRE(listener.reconfigurations().has_value());
    REQUIRE(listener.reconfigurations()->new_vendors().size() == 1);
    CHECK(listener.reconfigurations()->new_vendors().front().recommended_version == Slic3r::Semver{1, 0, 0});

    const auto reconfigurations = *listener.reconfigurations();

    listener.reset();
    fx.interactor().perform_reconfigurations(reconfigurations);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    CHECK(listener.performed());

    const fs::path installed_vendor_dir = fx.installed_path / k_repo_name / k_vendor_name;
    CHECK(fs::exists(installed_vendor_dir / "vendor.yaml"));
    CHECK(fs::exists(installed_vendor_dir / "printer.yaml"));
    CHECK(fs::exists(fx.installed_path / k_repo_name / (k_vendor_name + ".idx")));
    CHECK(read_file(installed_vendor_dir / "vendor.yaml").find("1.0.0") != std::string::npos);

    listener.reset();
    fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
    REQUIRE(fx.wait(listener));
    REQUIRE(listener.reconfigurations().has_value());
    CHECK(listener.reconfigurations()->empty());
}

TEST_CASE("PresetUpdater lists sources and persists selection", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    fx.interactor().list_repositories(true);
    REQUIRE(fx.wait(listener));
    REQUIRE(listener.repos().size() == 1);
    const auto& info = listener.repos().front();
    CHECK(info.descriptor.id == k_repo_name);
    CHECK(info.selected);
    const std::string uuid = info.descriptor.uuid;

    PresetUpdater::SharedPresetUpdaterRepositoryInfoVector desired;
    desired.push_back(PresetUpdater::SharedPresetUpdaterRepositoryInfo{info.descriptor, false});

    listener.reset();
    fx.interactor().update_repositories(true, desired);
    REQUIRE(fx.wait(listener));
    REQUIRE(listener.repos().size() == 1);
    CHECK_FALSE(listener.repos().front().selected);

    const std::string manifest = read_file(fx.shared_runtime_path / "RepositoryManifest.json");
    CHECK(manifest.find("\"selected\":false") != std::string::npos);
    CHECK(manifest.find(uuid) != std::string::npos);
}

TEST_CASE("PresetUpdater refreshes sources from the server on an empty selection", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    // An empty descriptor vector is a new query: the interactor syncs with the server
    // instead of applying a user selection.
    fx.interactor().update_repositories(true, PresetUpdater::SharedPresetUpdaterRepositoryInfoVector{});
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    REQUIRE(listener.repos().size() == 1);
    CHECK(listener.repos().front().descriptor.id == k_repo_name);
}

TEST_CASE("PresetUpdater adds and removes a local repository", "[preset_updater]")
{
    Fixture fx;

    CaptureListener listener(fx.interactor());

    const fs::path local_zip = fx.resources_profile_path / "local_repo.zip";
    REQUIRE(fs::exists(local_zip));

    fx.interactor().add_local_repository(true, local_zip, true);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());

    std::string local_uuid;
    for (const auto& repo : listener.repos()) {
        if (!repo.descriptor.unzipped_data_path.empty()) {
            local_uuid = repo.descriptor.uuid;
        }
    }
    REQUIRE_FALSE(local_uuid.empty());
    CHECK(fs::exists(fx.data_dir / "local_repositories" / local_uuid));

    listener.reset();
    fx.interactor().remove_local_repository(true, local_uuid);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    for (const auto& repo : listener.repos()) {
        CHECK(repo.descriptor.uuid != local_uuid);
    }
    CHECK_FALSE(fs::exists(fx.data_dir / "local_repositories" / local_uuid));
}

TEST_CASE("PresetUpdater verifies downloaded file hashes", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");
    fx.set_mutator([](const std::string& url, std::string& body, unsigned&) {
        if (url.ends_with("printer.yaml")) {
            body += "\n# corrupted by test\n";
        }
    });

    CaptureListener listener(fx.interactor());

    SECTION("ignore_hash=true stages the vendor despite corruption")
    {
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        CHECK(listener.reconfigurations()->new_vendors().size() == 1);
    }

    SECTION("ignore_hash=false rejects the vendor with a warning")
    {
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, false);
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        CHECK(listener.reconfigurations()->new_vendors().empty());
        CHECK_FALSE(listener.warnings().empty());
    }
}

TEST_CASE("PresetUpdater classifies multiple vendors in one repository", "[preset_updater]")
{
    Fixture fx;
    fx.put_resources("100");
    copy_dir_content_local(
        fx.resources_profile_path / "second_vendor" / k_repo_name,
        fx.resources_repo_path
    );
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    REQUIRE(listener.reconfigurations().has_value());

    const auto& new_vendors = listener.reconfigurations()->new_vendors();
    REQUIRE(new_vendors.size() == 2);

    std::vector<std::string> ids;
    for (const auto& reconf : new_vendors) {
        ids.push_back(reconf.vendor_id);
    }
    CHECK(std::find(ids.begin(), ids.end(), k_vendor_name) != ids.end());
    CHECK(std::find(ids.begin(), ids.end(), k_second_vendor) != ids.end());
}

TEST_CASE("PresetUpdater installs reconfigurations over an installed vendor", "[preset_updater]")
{
    Fixture fx;
    CaptureListener listener(fx.interactor());

    std::string target_version;

    SECTION("regular update raises the installed version")
    {
        fx.put_installed("100");
        fx.put_server("101");
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        REQUIRE(listener.reconfigurations()->regular_updates().size() == 1);
        CHECK(listener.reconfigurations()->regular_updates().front().recommended_version == Slic3r::Semver{1, 0, 1});
        target_version = "1.0.1";
    }

    SECTION("forced update replaces an app-incompatible installed version")
    {
        fx.put_installed("200");
        fx.put_server("201");
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        REQUIRE(listener.reconfigurations()->forced_updates().size() == 1);
        CHECK(listener.reconfigurations()->forced_updates().front().recommended_version == Slic3r::Semver{2, 0, 1});
        target_version = "2.0.1";
    }

    SECTION("forced downgrade replaces an app-incompatible newer installed version")
    {
        fx.put_installed("301");
        fx.put_server("300");
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        REQUIRE(listener.reconfigurations()->forced_downgrades().size() == 1);
        CHECK(listener.reconfigurations()->forced_downgrades().front().recommended_version == Slic3r::Semver{3, 0, 0});
        target_version = "3.0.0";
    }

    const auto reconfigurations = *listener.reconfigurations();

    listener.reset();
    fx.interactor().perform_reconfigurations(reconfigurations);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());
    CHECK(listener.performed());

    const fs::path installed_vendor_dir = fx.installed_path / k_repo_name / k_vendor_name;
    REQUIRE(fs::exists(installed_vendor_dir / "vendor.yaml"));
    CHECK(fs::exists(fx.installed_path / k_repo_name / (k_vendor_name + ".idx")));
    CHECK(read_file(installed_vendor_dir / "vendor.yaml").find(target_version) != std::string::npos);

    listener.reset();
    fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
    REQUIRE(fx.wait(listener));
    REQUIRE(listener.reconfigurations().has_value());
    CHECK(listener.reconfigurations()->empty());
}

TEST_CASE("PresetUpdater checks forced reconfigurations of installed profiles", "[preset_updater]")
{
    Fixture fx;
    CaptureListener listener(fx.interactor());

    SECTION("an app-compatible installed vendor needs no forced reconfiguration")
    {
        fx.put_installed("100");
        fx.interactor().check_forced_reconfigurations();
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        CHECK(listener.reconfigurations()->empty());
    }

    SECTION("an app-incompatible newer installed vendor is a forced downgrade")
    {
        fx.put_installed("301");
        fx.interactor().check_forced_reconfigurations();
        REQUIRE(fx.wait(listener));
        REQUIRE_FALSE(listener.got_error());
        REQUIRE(listener.reconfigurations().has_value());
        REQUIRE(listener.reconfigurations()->forced_downgrades().size() == 1);
        CHECK(listener.reconfigurations()->forced_downgrades().front().recommended_version == Slic3r::Semver{3, 0, 0});
        CHECK(listener.reconfigurations()->regular_updates().empty());
        CHECK(listener.reconfigurations()->forced_updates().empty());
        CHECK(listener.reconfigurations()->new_vendors().empty());
    }
}

TEST_CASE("PresetUpdater cleans up staged files and keeps installed profiles", "[preset_updater]")
{
    Fixture fx;
    fx.put_installed("100");
    fx.put_staged("100");

    const fs::path stray = fx.staged_path / "stray.tmp";
    {
        boost::nowide::ofstream f(stray);
        f << "leftover";
    }
    REQUIRE(fs::exists(fx.staged_path / k_repo_name));
    REQUIRE(fs::exists(stray));

    CaptureListener listener(fx.interactor());

    fx.interactor().cleanup_update_sync(true);
    REQUIRE(fx.wait(listener));
    REQUIRE_FALSE(listener.got_error());

    CHECK_FALSE(fs::exists(fx.staged_path / k_repo_name));
    CHECK_FALSE(fs::exists(stray));
    CHECK(fs::is_empty(fx.staged_path));

    const fs::path installed_vendor_dir = fx.installed_path / k_repo_name / k_vendor_name;
    CHECK(fs::exists(installed_vendor_dir / "vendor.yaml"));
    CHECK(fs::exists(fx.installed_path / k_repo_name / (k_vendor_name + ".idx")));
}

TEST_CASE("PresetUpdater ignores actions when the app config disables preset updates", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    fx.interactor().build_update_sync_and_reconfiguration_check(false, PresetUpdater::VerboseStyle::NoProgress, true);
    CHECK_FALSE(fx.wait(listener, 1s));
    CHECK_FALSE(listener.has_result());

    fx.interactor().list_repositories(false);
    CHECK_FALSE(fx.wait(listener, 1s));
    CHECK_FALSE(listener.has_result());

    fx.interactor().cleanup_update_sync(false);
    CHECK_FALSE(fx.wait(listener, 1s));
    CHECK_FALSE(listener.has_result());
}

TEST_CASE("PresetUpdater reports an unhandled worker exception as an error", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    CaptureListener listener(fx.interactor());

    SECTION("a std::exception is surfaced with its message")
    {
        fx.set_mutator([](const std::string&, std::string&, unsigned&) {
            throw std::runtime_error("injected worker failure");
        });
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE(listener.got_error());
        CHECK(listener.error_body().find("injected worker failure") != std::string::npos);
    }

    SECTION("a non-standard exception is surfaced as unknown")
    {
        fx.set_mutator([](const std::string&, std::string&, unsigned&) {
            throw 42;
        });
        fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
        REQUIRE(fx.wait(listener));
        REQUIRE(listener.got_error());
        CHECK(listener.error_body().find("unknown exception") != std::string::npos);
    }
}

TEST_CASE("PresetUpdater cancels a running operation and stays reusable", "[preset_updater]")
{
    Fixture fx;
    fx.put_server("100");

    std::atomic<bool> first_request_seen{false};
    fx.set_mutator([&first_request_seen](const std::string&, std::string&, unsigned&) {
        if (!first_request_seen.exchange(true)) {
            // Hold the worker inside its first network request so the main thread has a
            // deterministic window to request a stop before the post-sync checkpoint.
            std::this_thread::sleep_for(500ms);
        }
    });

    CaptureListener listener(fx.interactor());

    fx.interactor().build_update_sync_and_reconfiguration_check(true, PresetUpdater::VerboseStyle::NoProgress, true);
    std::this_thread::sleep_for(100ms);
    fx.interactor().on_notification_cancel();

    fx.dispatcher.dispatch_enqueued();
    CHECK(first_request_seen.load());
    CHECK_FALSE(listener.has_result());

    fx.set_mutator({});
    listener.reset();
    fx.interactor().list_repositories(true);
    REQUIRE(fx.wait(listener));
    CHECK_FALSE(listener.got_error());
    CHECK(listener.repos().size() == 1);
}
