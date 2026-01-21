#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <boost/filesystem/path.hpp>
#include "Slic3r/TestUtils/TestData.hpp"

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetSelectionCheck.hpp"
#include "Slic3r/Biz/Slicing/TestUtils.hpp"

namespace fs = boost::filesystem;


template <typename T>
void update_diff(
    Slic3r::Biz::Preset::PresetsSwitchStates& ret,
    Slic3r::Biz::Preset::PresetDiffOperation operation,
    std::optional<std::string> new_name,
    const T& original,
    const T& selected,
    const std::function<const Slic3r::Domain::ConfigBox&(const T&)>& getter,
    Slic3r::Domain::Preset::PresetKind kind,
    std::optional<size_t> slot = std::nullopt
)
{
    auto diff = getter(selected).diff_keys(getter(original));
    for (const auto& key : diff) {
        Slic3r::Biz::Preset::PresetSwitchState dest{
            .operation       = operation,
            .new_preset_name = new_name
        };
        const auto it = getter(selected).find(key);
        (it.is_override ? dest.overrides : dest.items).insert({key, it.item->value()});
        ret.emplace(Slic3r::Biz::Preset::PresetSwitchKindId{kind, slot}, std::move(dest));
    }
}

class TestPresetDialogManager : Slic3r::Biz::Preset::IPresetDialogManager
{
public:
    PresetsSwitchStates show_unsaved_changes_dialog(
        const std::string& dialog_name,
        const Slic3r::Domain::ConfigPack& config_original,
        const Slic3r::Domain::ConfigPack& config_selected,
        Slic3r::Domain::ConfigPack* config_new_selected,
        const Slic3r::Biz::Preset::PresetSelectionNames& preset_names,
        const Slic3r::Biz::Preset::PresetSelectionNames& preset_names_new,
        const Slic3r::Biz::Preset::PresetInteractor& preset_interactor
    ) override
    {
        PresetsSwitchStates ret;
        std::visit(
            Slic3r::Domain::overloaded{
                [&](const Slic3r::Domain::ConfigPackFDM& original)
                {
                    const auto& selected = std::get<Slic3r::Domain::ConfigPackFDM>(config_selected);
                    update_diff<Slic3r::Domain::ConfigPackFDM>(
                        ret,
                        operation,
                        new_name,
                        original,
                        selected,
                        [](const auto& p) -> const Slic3r::Domain::ConfigBox& { return p.printer; },
                        Slic3r::Domain::Preset::PresetKind::FdmPrinter
                    );
                    update_diff<Slic3r::Domain::ConfigPackFDM>(
                        ret,
                        operation,
                        new_name,
                        original,
                        selected,
                        [](const auto& p) -> const Slic3r::Domain::ConfigBox& { return p.print; },
                        Slic3r::Domain::Preset::PresetKind::FdmPrint
                    );
                    for (size_t i = 0; i < original.tool.size(); i++) {
                        update_diff<Slic3r::Domain::ConfigPackFDM>(
                            ret,
                            operation,
                            new_name,
                            original,
                            selected,
                            [&](const auto& p) -> const Slic3r::Domain::ConfigBox&
                            { return p.tool[i]; },
                            Slic3r::Domain::Preset::PresetKind::FdmToolPrint, i
                        );
                        update_diff<Slic3r::Domain::ConfigPackFDM>(
                            ret,
                            operation,
                            new_name,
                            original,
                            selected,
                            [&](const auto& p) -> const Slic3r::Domain::ConfigBox&
                            { return p.filament[i]; },
                            Slic3r::Domain::Preset::PresetKind::FdmMaterial, i
                        );
                    }
                },
                [&](const Slic3r::Domain::ConfigPackSLA& original)
                {
                    const auto& selected = std::get<Slic3r::Domain::ConfigPackSLA>(config_selected);
                    update_diff<Slic3r::Domain::ConfigPackSLA>(
                        ret,
                        operation,
                        new_name,
                        original,
                        selected,
                        [](const auto& p) -> const Slic3r::Domain::ConfigBox&
                        { return p.sla_printer_settings; },
                        Slic3r::Domain::Preset::PresetKind::SlaPrinter
                    );
                    update_diff<Slic3r::Domain::ConfigPackSLA>(
                        ret,
                        operation,
                        new_name,
                        original,
                        selected,
                        [](const auto& p) -> const Slic3r::Domain::ConfigBox&
                        { return p.sla_print_settings; },
                        Slic3r::Domain::Preset::PresetKind::SlaPrint
                    );
                    update_diff<Slic3r::Domain::ConfigPackSLA>(
                        ret,
                        operation,
                        new_name,
                        original,
                        selected,
                        [](const auto& p) -> const Slic3r::Domain::ConfigBox&
                        { return p.sla_material_settings; },
                        Slic3r::Domain::Preset::PresetKind::SlaMaterial
                    );
                }
            },
            config_original
        );
        return ret;
    }

    Slic3r::Biz::Preset::PresetDiffOperation operation = Slic3r::Biz::Preset::PresetDiffOperation::Save;
    std::string new_name{"test"};
};



struct BaseProjectInteractorFixture
{
    Slic3r::Domain::Workbench workbench;
    Slic3r::App::Platform::StdMainThreadDispatcher main_thread_dispatcher;
    Slic3r::Tests::MockThumbnailImageGenerator thumbnail_image_generator;
    Slic3r::Biz::ProjectInteractor project_interactor{workbench, main_thread_dispatcher, thumbnail_image_generator};
    TestPresetDialogManager preset_dialog_manager;

    virtual ~BaseProjectInteractorFixture()
    {
        main_thread_dispatcher.close();
    }
};

struct LoadedProjectInteractorFixture : BaseProjectInteractorFixture
{
    const Slic3r::Biz::Preset::IO::BundlePaths bundle_paths{
        Slic3r::Biz::Preset::IO::BundlePaths::make_test_runtime(Tests::get_datadir())
    };

    LoadedProjectInteractorFixture()
    {
        project_interactor.preset_interactor().load_preset_bundle(bundle_paths);
    }

    ~LoadedProjectInteractorFixture() override
    {
        fs::remove_all(bundle_paths.user_bundle_path);
    }
};

template <typename T>
const T* find_if(const Slic3r::Biz::IObservableList<T>& list, const std::function<bool(const T&)>& predicate)
{
    for (size_t i = 0, n = list.size(); i < n; ++i) {
        if (predicate(list.at(i)))
            return &list.at(i);
    }
    return nullptr;
}

TEST_CASE_METHOD(LoadedProjectInteractorFixture, "Preset Interactor Tests", "[PresetInteractor]")
{
    project_interactor.new_project();
    auto& preset_interactor = project_interactor.preset_interactor();
    const auto& printer_items = preset_interactor.printer_presets();
    REQUIRE(printer_items.items().size() > 0);

    auto switch_pritner_and_verify = [&](const std::string& printer_name)     {
        std::optional<size_t> printer_idx;
        for (size_t i = 0, n = printer_items.items().size(); i < n; ++i) {
            const auto& item = printer_items.items().at(i);
            if (item.hw_printer_config_name == printer_name) {
                printer_idx = i;
                break;
            }
        }
        REQUIRE(printer_idx.has_value() == true);
        const auto& selected_printer = printer_items.items().at(printer_idx.value());
        preset_interactor.select_printer_preset(selected_printer.hw_printer_config_id, selected_printer.id);

        const auto selected_printer_idx = printer_items.selected_index();
        REQUIRE(selected_printer_idx != Slic3r::Domain::INVALID_ID);
        REQUIRE(printer_items.items().at(selected_printer_idx).hw_printer_config_name == printer_name);
    };


    // Validate precondition: selected printer
    {
        const auto selected_printer_idx = printer_items.selected_index();
        REQUIRE(selected_printer_idx != Slic3r::Domain::INVALID_ID);
        REQUIRE(printer_items.items().at(selected_printer_idx).hw_printer_config_name.starts_with("CORE One"));
    }

    // Make sure the printer is switched to Core ONE 0.4 HF
    switch_pritner_and_verify("CORE One 0.4 HF");

    // Switch print to 0.20mm
    {
        preset_interactor.select_print_preset("0.20mm");
        const auto& print_presets = preset_interactor.print_presets();
        const auto print_preset_idx = print_presets.selected_index();
        REQUIRE(print_presets.items().at(print_preset_idx).id == "0.20mm");
    }

    // Switch printer to CORE One 0.6 HF
    switch_pritner_and_verify("CORE One 0.6 HF");

    // Verify that printer preset stays 0.20mm
    {
        const auto& print_presets = preset_interactor.print_presets();
        const auto print_preset_idx = print_presets.selected_index();
        REQUIRE(print_presets.items().at(print_preset_idx).id == "0.20mm");
    }

    {
        auto user_preset = find_if<Slic3r::Biz::Preset::PresetItem>(preset_interactor.print_presets().items(), [](const auto& p) -> bool { return p.name == "test"; });
        REQUIRE(user_preset == nullptr);

        preset_interactor.set_preset_value(
            Slic3r::Domain::FDMConfigLocation::Print,
            0,
            "brim_width",
            [](auto& p) { p.set(2.0); }
        );
        bool can_select = Slic3r::Biz::Preset::PresetSelectionCheck::can_select_print_preset(preset_interactor, "0.20mm");
        REQUIRE(can_select);
        const auto& hw_config = preset_interactor.current_printer_config();
        fs::path saved_preset_path = bundle_paths.user_preset_dir_path(hw_config.repo_id, hw_config.vendor_id) / ("print-" + preset_dialog_manager.new_name + ".yaml");
        REQUIRE(fs::exists(saved_preset_path));

        user_preset = find_if<Slic3r::Biz::Preset::PresetItem>(preset_interactor.print_presets().items(), [](const auto& p) -> bool { return p.name == "test"; });
        REQUIRE(user_preset != nullptr);

        preset_interactor.load_preset_bundle(bundle_paths);
        user_preset = find_if<Slic3r::Biz::Preset::PresetItem>(preset_interactor.print_presets().items(), [](const auto& p) -> bool { return p.name == "test"; });
        REQUIRE(user_preset != nullptr);
    }
}

