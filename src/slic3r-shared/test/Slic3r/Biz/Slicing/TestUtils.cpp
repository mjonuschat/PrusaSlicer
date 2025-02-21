#include "TestUtils.hpp"

#include <fstream>
#include <boost/filesystem.hpp>

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
    config.set("skirts", 0);
    config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology", true)->value = ptFFF;
    return config;
}

ModelOnBed::ModelOnBed(Model&& model, DynamicPrintConfig&& config)
    : model{std::move(model)}, config{std::move(config)}, bed_instance{ModelOnBed::bed}
{
    for (ModelObject* object : this->model.objects) {
        for (ModelInstance* instance : object->instances) {
            this->bed_instance.model_instances().push_back(instance);
        }
    }
}

Domain::Bed ModelOnBed::bed{};

ModelOnBed get_cubes_model(const int count, const int row_size) {
    return {generate_cubes(count, row_size), get_config()};
}

std::ostream& operator<<(std::ostream& output, const StatusEvent& status_event) {
    return output << "{status: " << status_event.status
                  << ", slicing_id: " << status_event.slicing_id << "}";
}

bool operator==(const StatusEvent& a, const StatusEvent& b) {
    return a.status == b.status && a.slicing_id == b.slicing_id;
}

using StatusEvents = std::vector<StatusEvent>;

[[nodiscard]] bool wait_for_status(
    Biz::Platform::IMainThreadDispatcher& dispatcher,
    const StatusListener& status_listener,
    const std::chrono::seconds timeout,
    const std::function<bool(StatusEvents)>& condition
)
{
    using namespace std::chrono_literals;
    using std::chrono::high_resolution_clock;
    using Biz::Platform::PlatformServices;

    const auto start{high_resolution_clock::now()};
    while(true) {
        dispatcher.dispatch_enqueued();
        const std::vector<StatusEvent> status_events{status_listener.status_events};
        if (!status_events.empty() && condition(status_events)) {
            return true;;
        }
        const auto now{high_resolution_clock::now()};
        if (duration_cast<std::chrono::seconds>(now - start) > timeout) {
            return false;
        }
        std::this_thread::sleep_for(1ms);
    }
}

using Biz::Slicing::FDMResult;
using Biz::Slicing::SlicingId;
using Biz::Slicing::IStatusListener;

void ResultListener::on_fdm_result_changed(
    std::shared_ptr<FDMResult> result, const SlicingId id
)
{
    gcodes[id.bed_instance_id] = result->gcode.str();
}

SlicingFixture::SlicingFixture() {
    slicing.on_selected_project_changed(0);
    slicing.add_listener<IStatusListener>(&status_listener);
}

SlicingFixture::~SlicingFixture() {
    dispatcher.close();
}

}
