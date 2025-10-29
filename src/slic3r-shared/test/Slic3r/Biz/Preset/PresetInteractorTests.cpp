#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <boost/filesystem/path.hpp>
#include "Slic3r/TestUtils/TestData.hpp"

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Slicing/TestUtils.hpp"

struct BaseProjectInteractorFixture
{
    Slic3r::Domain::Workbench workbench;
    Slic3r::App::Platform::StdMainThreadDispatcher main_thread_dispatcher;
    Slic3r::Tests::MockThumbnailImageGenerator thumbnail_image_generator;
    Slic3r::Biz::ProjectInteractor project_interactor{workbench, main_thread_dispatcher, thumbnail_image_generator};

    virtual  ~BaseProjectInteractorFixture()
    {
        main_thread_dispatcher.close();
    }
};

struct LoadedProjectInteractorFixture : BaseProjectInteractorFixture
{
    LoadedProjectInteractorFixture()
    {
        namespace fs = boost::filesystem;
        auto data_dir              = Tests::get_datadir();
        fs::path preset_bundle_dir = data_dir / "presets";
        fs::path config_dir        = data_dir / "configs";

        project_interactor.preset_interactor().load_preset_bundle(
            preset_bundle_dir.string(),
            config_dir.string()
        );

    }
};

TEST_CASE_METHOD(LoadedProjectInteractorFixture, "Preset Interactor Tests", "[PresetInteractor]")
{
    project_interactor.new_project();
    auto& preset_interactor = project_interactor.preset_interactor();
    const auto& printer_items = preset_interactor.printer_presets();
    REQUIRE(printer_items.items().size() > 0);

    // Validate precondition: selected printer
    {
        const auto selected_printer_idx = printer_items.selected_index();
        REQUIRE(selected_printer_idx != Slic3r::Domain::INVALID_ID);
        REQUIRE(printer_items.items().at(selected_printer_idx).hw_printer_config_name == "CORE One 0.4 HF");
    }

    // Switch print to 0.20mm
    {
        preset_interactor.select_print_preset("0.20mm");
        const auto& print_presets = preset_interactor.print_presets();
        const auto print_preset_idx = print_presets.selected_index();
        REQUIRE(print_presets.items().at(print_preset_idx).id == "0.20mm");
    }

    // Switch printer to CORE One 0.6 HF
    {
        std::optional<size_t> printer_idx;
        for (size_t i = 0, n = printer_items.items().size(); i < n; ++i) {
            const auto& item = printer_items.items().at(i);
            if (item.hw_printer_config_name == "CORE One 0.6 HF") {
                printer_idx = i;
                break;;
            }
        }
        REQUIRE(printer_idx.has_value() == true);
        const auto& selected_printer = printer_items.items().at(printer_idx.value());
        preset_interactor.select_printer_preset(selected_printer.hw_printer_config_id, selected_printer.id);

        const auto selected_printer_idx = printer_items.selected_index();
        REQUIRE(selected_printer_idx != Slic3r::Domain::INVALID_ID);
        REQUIRE(printer_items.items().at(selected_printer_idx).hw_printer_config_name == "CORE One 0.6 HF");
    }

    // Verify that printer preset stays 0.20mm
    {
        const auto& print_presets = preset_interactor.print_presets();
        const auto print_preset_idx = print_presets.selected_index();
        REQUIRE(print_presets.items().at(print_preset_idx).id == "0.20mm");
    }
}

