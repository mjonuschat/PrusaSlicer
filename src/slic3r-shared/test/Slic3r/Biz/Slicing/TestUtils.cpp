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
}
