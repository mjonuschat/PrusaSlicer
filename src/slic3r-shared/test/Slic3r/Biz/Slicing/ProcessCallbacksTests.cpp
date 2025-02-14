#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <boost/filesystem/operations.hpp>
#include <Slic3r/Biz/Slicing/BackgroundProcess.hpp>
#include <regex>
#include <fstream>

#include <Slic3r/App/Platform/StdMainThreadDispatcher.hpp>

#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/Biz/Slicing/GCodeUtils.hpp"

using namespace Catch;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Biz::Slicing::BackgroundProcess;
using Slic3r::Biz::Slicing::IProcessCallbacks;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Tests::is_gcode_sane;
using Slic3r::Tests::get_config;
using Slic3r::Tests::generate_cubes;
using Slic3r::Tests::get_cubes_filament_used;
using Slic3r::Biz::Slicing::ProjectBedId;
using Slic3r::Biz::Print::WipeTowerGeometry;

struct SharedState {
    std::string gcode{};
    FDMStatistics statistics{};
    Status status{};
    WipeTowerGeometry wipe_tower_geometry{};
};

struct CallbacksHandler : IProcessCallbacks {
    void on_fdm_result(FDMResult&& result, FDMStatistics&& statistics, const ProjectBedId) override
    {
        std::ifstream file{result.filename};
        std::stringstream buffer;
        buffer << file.rdbuf();

        SharedState state{this->get_state()};
        state.gcode = buffer.str();
        state.statistics = std::move(statistics);
        this->set_state(state);
        boost::system::error_code error_code;
        boost::filesystem::remove(result.filename, error_code);
    }

    void on_status(const Status status, const ProjectBedId) override {
        SharedState state{this->get_state()};
        state.status = status;
        this->set_state(state);
    }

    void on_wipe_tower_geometry(WipeTowerGeometry&& geometry, const ProjectBedId) override {
        SharedState state{this->get_state()};
        state.wipe_tower_geometry = std::move(geometry);
        this->set_state(state);
    }

    Status get_status(const ProjectBedId) const override {
        return this->get_state().status;
    }

    SharedState get_state() const {
        const std::scoped_lock lock{m_mutex};
        return m_state;
    }

    void reset_state() {
        this->set_state(SharedState{});
    }

    void wait_for_status(const seconds timeout, const std::function<bool(Status)>& condition) {
        using namespace std::chrono_literals;

        const auto start{high_resolution_clock::now()};
        while(true) {
            if (condition(get_state().status)) {
                break;
            }
            const auto now{high_resolution_clock::now()};
            if (duration_cast<seconds>(now - start) > timeout) {
                throw std::runtime_error{"Waiting for result timed out!"};
            }
            std::this_thread::sleep_for(1ms);
        }
    }

private:
    mutable std::mutex m_mutex;
    SharedState m_state;

    void set_state(const SharedState &state) {
        const std::scoped_lock lock{m_mutex};
        m_state = state;
    }
};

TEST_CASE("Background process slicing waits for update", "[slicing][slicing-callbacks]")
{
    using namespace std::chrono_literals;

    CallbacksHandler callbacks_handler;
    BackgroundProcess process{callbacks_handler, generate_cubes(0, 5), get_config(), ProjectBedId{}};
    Slic3r::Model model{generate_cubes(7, 5)};
    process.update(model, get_config());
    process.slice();
    callbacks_handler.wait_for_status(10s, [](const Status status) {
        return status == Status::Finished;
    });

    const auto error{is_gcode_sane(callbacks_handler.get_state().gcode, model)};
    INFO((error ? *error : ""));
    CHECK(!error);

    const double filament_used{callbacks_handler.get_state().statistics.total_used_filament};
    REQUIRE(filament_used == Approx(get_cubes_filament_used(model)).margin(10.0));
}
