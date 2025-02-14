#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Slic3r/Biz/Slicing/BackgroundProcess.hpp>

#include "Slic3r/Biz/Slicing/TestUtils.hpp"

using namespace Catch;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Biz::Slicing::BackgroundProcess;
using Slic3r::Biz::Print::IPrint;
using Slic3r::Biz::Print::ApplyStatus;
using Slic3r::Biz::Print::WipeTowerGeometry;
using Slic3r::Biz::Slicing::IProcessCallbacks;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Tests::precise_sleep;

using LogEntry = std::pair<time_point, Status>;
using LogEntries = std::vector<LogEntry>;
using Slic3r::Biz::Slicing::ProjectBedId;

struct StatusLog : IProcessCallbacks
{
    StatusLog() = default;
    StatusLog(const LogEntries &entries): statuses{entries} {}

    LogEntries statuses{};

    void on_status(const Status status, const ProjectBedId) override {
        statuses.emplace_back(high_resolution_clock::now(), status);
        m_status.store(status);
    }

    Status get_status(const ProjectBedId) const override {
        return m_status.load();
    }

    void on_fdm_result(FDMResult&& fdm_result, FDMStatistics&&, const ProjectBedId) override {}
    void on_wipe_tower_geometry(WipeTowerGeometry&&, const ProjectBedId) override {}

    std::vector<std::pair<time_point, Status>> get_entries(const Status status) const
    {
        std::vector<std::pair<time_point, Status>> result;
        std::copy_if(
            this->statuses.begin(), this->statuses.end(), std::back_inserter(result),
            [&](const std::pair<time_point, Status>& log_entry) {
                return log_entry.second == status;
            }
        );
        return result;
    }

private:
    std::atomic<Status> m_status;
};

std::ostream& operator<<(std::ostream& out, const StatusLog& log)
{
    if (log.statuses.empty()) {
        out << "empty log" << std::endl;
        return out;
    }
    const auto initial_entry{log.statuses.front()};
    for (const LogEntry& entry : log.statuses) {
        out << duration_cast<milliseconds>(entry.first - initial_entry.first).count() << "ms";
        out << " " << entry.second << std::endl;
    }
    return out;
}

template<typename T = milliseconds>
bool is_equal(const StatusLog& a, const StatusLog& b, const std::optional<T> tolerance = std::nullopt)
{
    if (a.statuses.size() != b.statuses.size()) {
        return false;
    }
    if (a.statuses.empty()) {
        return true;
    }

    const auto& a_first_time{a.statuses.front().first};
    const auto& b_first_time{b.statuses.front().first};
    for (std::size_t i{}; i < a.statuses.size(); ++i) {
        const LogEntry a_entry{a.statuses.at(i)};
        const LogEntry b_entry{b.statuses.at(i)};
        if (a_entry.second != b_entry.second) {
            return false;
        }
        if (tolerance) {
            const auto a_duration{duration_cast<T>(a_entry.first - a_first_time)};
            const auto b_duration{duration_cast<T>(b_entry.first - b_first_time)};
            const auto difference{a_duration.count() - b_duration.count()};
            if (std::abs(difference) > tolerance->count()) {
                return false;
            }
        }
    }
    return true;
}

milliseconds measure_execution_time(const std::function<void()>& func)
{
    const auto start{high_resolution_clock::now()};
    func();
    const auto end{high_resolution_clock::now()};
    return duration_cast<milliseconds>(end - start);
}

class MockPrint : public IPrint
{
public:
    ApplyStatus update(const Slic3r::Model&, Slic3r::DynamicPrintConfig) override
    {
        precise_sleep(this->apply_time);
        return ApplyStatus::changed;
    }
    void slice() override
    {
        const auto start{high_resolution_clock::now()};
        while (true) {
            if (this->stop_token.stop_requested()) {
                precise_sleep(this->delay_after_stop);
                throw Slic3r::CanceledException{};
            }

            const auto current_time{high_resolution_clock::now()};
            const auto duration{duration_cast<milliseconds>(current_time - start)};

            if (duration > processing_time) {
                return;
            }
        }
    }
    bool empty() const override { return false; }

    milliseconds apply_time{};
    milliseconds delay_after_stop{};
    milliseconds processing_time{};
};

TEST_CASE("Test bsp slice() returns immediately", "[bsp][slicing-actions]")
{
    using namespace std::chrono_literals;

    const auto processing_time{50ms};
    auto print{std::make_unique<MockPrint>()};
    print->processing_time = processing_time;

    StatusLog log;
    const Slic3r::Model model{};
    BackgroundProcess
        bsp{std::move(print), log, model, Slic3r::DynamicPrintConfig::full_print_config(), ProjectBedId{}};

    const auto execution_time{measure_execution_time([&]() { bsp.slice(); })};
    INFO("bsp.slice() exectuion time: " + std::to_string(execution_time.count()));
    REQUIRE(execution_time <= 5ms); // It should return imediatly
                                    //
    // Make sure there is plenty of time for the processing to finish.
    precise_sleep(processing_time * 2);

    LogEntries statuses{
        LogEntry{0ms, Status::Updating}, LogEntry{0ms, Status::Modified},
        LogEntry{0ms, Status::Running}, LogEntry{processing_time, Status::Finished}};
    const StatusLog expected_log{statuses};
    INFO(log);
    INFO("!=");
    INFO(expected_log);
    CHECK(is_equal<milliseconds>(log, expected_log, 5ms));
}

TEST_CASE("Test bsp stop() returns immediately", "[bsp][slicing-actions]")
{
    using namespace std::chrono_literals;

    const auto processing_time{50ms};
    const auto delay_after_stop{35ms};
    auto print{std::make_unique<MockPrint>()};
    print->processing_time = processing_time;
    print->delay_after_stop = delay_after_stop;

    StatusLog log;
    const Slic3r::Model model{};
    BackgroundProcess
        bsp{std::move(print), log, model, Slic3r::DynamicPrintConfig::full_print_config(), ProjectBedId{}};

    bsp.slice();
    const auto time_before_stop{20ms};
    precise_sleep(time_before_stop);
    const auto execution_time{measure_execution_time([&]() { bsp.stop(); })};
    INFO("bsp.stop() exectuion time: " + std::to_string(execution_time.count()));
    REQUIRE(execution_time <= 5ms); // It should return imediatly
    for (std::size_t _{}; _ < 5; _++) {
        // Consecutive stops should not increase the waiting time,
        // nor fire status event.
        precise_sleep(2ms);
        bsp.stop();
    }
    bsp.slice();

    // Make sure there is plenty of time for the processing to finish.
    precise_sleep(processing_time * 2);

    std::vector<LogEntry> statuses{
        LogEntry{0ms, Status::Updating},
        LogEntry{0ms, Status::Modified},
        LogEntry{0ms, Status::Running},
        LogEntry{time_before_stop, Status::Stopping},
        LogEntry{time_before_stop + delay_after_stop, Status::Modified},
        LogEntry{time_before_stop + delay_after_stop, Status::Running},
        LogEntry{time_before_stop + delay_after_stop + processing_time, Status::Finished},
    };
    const StatusLog expected_log{statuses};
    INFO(log);
    INFO("!=");
    INFO(expected_log);
    CHECK(is_equal<milliseconds>(log, expected_log, 5ms));
}

TEST_CASE("Test bsp update() updates status", "[bsp][slicing-actions]")
{
    using namespace std::chrono_literals;

    const auto processing_time{50ms};
    const auto delay_after_cancel{50ms};
    const auto apply_time{20ms};
    auto print{std::make_unique<MockPrint>()};
    print->processing_time = processing_time;
    print->delay_after_stop = delay_after_cancel;
    print->apply_time = apply_time;

    StatusLog log;
    std::optional<BackgroundProcess> optional_bsp;

    const Slic3r::Model model{};
    const auto execution_time{measure_execution_time([&]() {
        optional_bsp
            .emplace(std::move(print), log, model, Slic3r::DynamicPrintConfig::full_print_config(), ProjectBedId{});
    })};
    INFO("bsp.update() exectuion time: " + std::to_string(execution_time.count()));
    REQUIRE((execution_time - apply_time) < 5ms); // Update blocks the ui thread.

    // Let the apply finish
    precise_sleep(apply_time * 2);

    std::vector<LogEntry> statuses{
        LogEntry{0ms, Status::Updating},
        LogEntry{apply_time, Status::Modified},
    };
    const StatusLog expected_log{statuses};
    INFO(log);
    INFO("!=");
    INFO(expected_log);
    CHECK(is_equal<milliseconds>(log, expected_log, 5ms));
}
