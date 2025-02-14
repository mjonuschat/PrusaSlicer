#include "TestUtils.hpp"

namespace Slic3r::Tests {
void precise_sleep(const std::chrono::milliseconds duration) {
    const auto start = std::chrono::high_resolution_clock::now();
    const auto nano_duration{duration_cast<std::chrono::nanoseconds>(duration)};
    while (std::chrono::high_resolution_clock::now() - start < nano_duration) {
        // CPU intensive but precise. Suitable just for testing.
    }
}

Slic3r::Model generate_cubes(const int count, const int row_size)
{
    const double size{20};
    Slic3r::Model model;
    for (int i{}; i < count; ++i) {
        const int row{i / row_size};
        const int column{i % row_size};
        Slic3r::TriangleMesh cube_mesh = Slic3r::make_cube(size, size, size);
        cube_mesh.translate(column * (size + 5), row * (size + 5), 0);

        Slic3r::ModelObject* model_object = model.add_object();
        model_object->add_volume(cube_mesh);
        model_object->add_instance();
        model_object->ensure_on_bed();
    }
    return model;
}


double get_cubes_filament_used(const Slic3r::Model &model) {
    return model.objects.size() * 1483.0;
}

Slic3r::DynamicPrintConfig get_config() {
    auto config{Slic3r::DynamicPrintConfig::full_print_config()};
    config.opt_int("skirts") = 0;
    return config;
}

bool operator==(const StatusEvent& a, const StatusEvent& b) {
    return a.status == b.status && a.project_bed_id == b.project_bed_id;
}

using StatusEvents = std::vector<StatusEvent>;

void wait_for_status(
    Slic3r::Biz::Platform::IMainThreadDispatcher& dispatcher,
    const StatusListener& status_listener,
    const std::chrono::seconds timeout,
    const std::function<bool(StatusEvents)>& condition
)
{
    using namespace std::chrono_literals;
    using std::chrono::high_resolution_clock;

    const auto start{high_resolution_clock::now()};
    while(true) {
        dispatcher.dispatch_enqueued();
        const std::vector<StatusEvent> status_events{status_listener.status_events};
        if (!status_events.empty() && condition(status_events)) {
            break;
        }
        const auto now{high_resolution_clock::now()};
        REQUIRE(duration_cast<std::chrono::seconds>(now - start) < timeout);
        std::this_thread::sleep_for(1ms);
    }
}

Slic3r::Biz::Slicing::SlicingInteractor init_slicing_interactor(Slic3r::Biz::Platform::IMainThreadDispatcher &dispatcher) {
    Slic3r::Biz::Platform::PlatformServices::instance().set_main_thread_dispatcher(&dispatcher);
    return {};
}

SlicingFixture::SlicingFixture(): slicing(init_slicing_interactor(dispatcher))  {
    slicing.on_selected_project_changed(0);
    slicing.add_status_listener(&status_listener);
}

}
