#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Uuid.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Biz/Preset/HwConfigEvaluator.hpp"
#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/IO/BundleLoader.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include "Slic3r/Version.hpp"

#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
#include "boost/filesystem/path.hpp"
#include <Slic3r/Log.hpp>

using Slic3r::Domain::Vec2d;

namespace Slic3r::Biz::Preset {

namespace {

void dump_ep_info(const Domain::Preset::EvaluatedPrinterPreset& preset)
{
    SPDLOG_INFO("HwConfig: {} ({})", preset.hw_config.name, preset.hw_config.id);
    SPDLOG_INFO("Printer: {} ({})", preset.preset.name, preset.preset.id);
    for (const auto& p : preset.prints) {
        SPDLOG_INFO("-------------------------------------------------");
        SPDLOG_INFO("Print: {} ({})", p.preset.name, p.preset.id);
        size_t idx = 0;
        for (const auto& tvs : p.tools) {
            if (idx > 0)
                SPDLOG_INFO("................................................");
            idx++;
            SPDLOG_INFO("Tool {}", idx);
            for (const auto& t : tvs) {
                SPDLOG_INFO("- {} ({})", t.preset.name, t.preset.id);
            }

            size_t tool_idx = 0;
            SPDLOG_INFO("-------------------------------------------------");
            SPDLOG_INFO("Materials:");
            SPDLOG_INFO("-------------------------------------------------");
            for (const auto& tool_mats : p.materials) {
                if (tool_idx > 0)
                    SPDLOG_INFO("................................................");
                tool_idx++;
                SPDLOG_INFO("Tool {}", tool_idx);
                for (const auto& mat : tool_mats) {
                    SPDLOG_INFO("- {} ({})", mat.preset.name, mat.preset.id);
                }
            }
            SPDLOG_INFO("-------------------------------------------------");
        }
    }
}

} // namespace

PresetInteractor::PresetInteractor(Domain::Workbench& workbench, Scene::SceneInteractor& scene_interactor) :
    m_workbench(workbench),
    m_object_settings_interactor(m_object_settings_interactor_accessor, m_workbench, scene_interactor)
{
    ConfigBoxInteractor::SetAccessor printer_accessor;
    m_printer_cbi                   = ConfigBoxInteractor(printer_accessor, nullptr);
    m_cbi_accessors[&m_printer_cbi] = std::move(printer_accessor);

    ConfigBoxInteractor::SetAccessor print_accessor;
    m_print_cbi                   = ConfigBoxInteractor(print_accessor, nullptr);
    m_cbi_accessors[&m_print_cbi] = std::move(print_accessor);
}

void PresetInteractor::load_preset_bundle(
    const std::string& preset_bundle_path,
    const std::string& config_path
)
{
    std::optional<Domain::Preset::Bundle> preset_bundle_opt;
#ifndef NDEBUG
    namespace fs = boost::filesystem;
    const std::string bundle_cache_filename = fs::path(fs::path(Slic3r::data_dir()) / "cache" / "bundle_cache").string();
    preset_bundle_opt = IO::deserialize_bundle(bundle_cache_filename, preset_bundle_path, config_path, Slic3r::VERSION);
#endif
    if (! preset_bundle_opt) {
        preset_bundle_opt = std::make_optional(IO::load_bundle(preset_bundle_path, config_path));
        Domain::Preset::Bundle& preset_bundle = *preset_bundle_opt;

        // TODO: remove this when config wizard is ready
        if (preset_bundle.printer_configs.empty()) {
            HwConfigEvaluator config_eval;
            for (const auto& vendor : {"PrusaResearch", "PrusaResearchSLA"}) {
                auto vendor_bundle_it = preset_bundle.vendor_bundles.find(vendor);
                ASSERT(vendor_bundle_it != preset_bundle.vendor_bundles.end());
                auto& vendor_bundle = vendor_bundle_it->second;
                for (const auto& hw_printer_template : vendor_bundle.vendor_data.printer_configs) {
                    auto printer_config = config_eval.create_printer_config(
                        hw_printer_template,
                        vendor_bundle.vendor_data
                    );
                    preset_bundle.printer_configs.emplace(printer_config.id, printer_config);
                    vendor_bundle.printer_configs.emplace_back(std::move(printer_config));
                }
            }
        }

        preset_bundle.evaluated_presets.clear();
        std::mutex mut;

        for (const auto& [vendor_id, vendor_bundle] : preset_bundle.vendor_bundles) {
            PresetEvaluator preset_evaluator{vendor_bundle.presets};
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, vendor_bundle.printer_configs.size()),
                [&](const tbb::blocked_range<size_t>& r) {
                    for (size_t i = r.begin(); i != r.end(); ++i) {
                        try {
                            auto epps = preset_evaluator.evaluate(vendor_bundle.printer_configs[i]);
                            for (auto& epp : epps) {
                                std::lock_guard<std::mutex> guard(mut);
                                //dump_ep_info(epp);
                                preset_bundle.evaluated_presets[epp.hw_config.id].emplace_back(std::move(epp));
                            }
                        } catch (const std::exception& e) {
                            SPDLOG_ERROR("{}", e.what());
                        }
                    }
                }
            );
        }
    #ifndef NDEBUG
        IO::serialize_bundle(bundle_cache_filename, *preset_bundle_opt, preset_bundle_path, config_path, Slic3r::VERSION);
    #endif
    }

    // do not save it now, as we create it anyway again
    // IO::save_bundle_configs(preset_bundle, config_path);

    m_workbench.set_preset_bundle(std::move(*preset_bundle_opt));
    if (m_selected_project_id != Domain::INVALID_ID)
        fill_printer_presets();
}

const PresetInteractorConfigContainerContext& PresetInteractor::config_container_context(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
) const
{
    const auto& project_ctx = get_project_context(project_id)->second;
    const auto& cccs        = project_ctx.config_containers;
    return cccs.find(config_container_id)->second;
}

const PresetInteractorConfigContainerContext&
PresetInteractor::selected_config_container_context() const
{
    const auto& project_ctx = get_project_context(m_selected_project_id)->second;
    const auto& cccs        = project_ctx.config_containers;
    return cccs.find(project_ctx.selected_config_container_id)->second;
}

PresetInteractorConfigContainerContext& PresetInteractor::initialize_config_container_context(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    auto& project_context{get_or_create_project_context(project_id)};
    PresetInteractorConfigContainerContext ccc{config_container_id};
    const auto [it, inserted]{
        project_context.config_containers.insert({config_container_id, std::move(ccc)})
    };
    ASSERT(inserted);
    return it->second;
}

void PresetInteractor::initialize_config_container(Domain::ConfigContainer& cc)
{
    const static std::string selected_printer_name =
        "CORE One 0.4 HF"; //"SL1S SPEED";//"Prusa MK4S";
    const auto& preset_bundle     = m_workbench.preset_bundle();
    const auto& evaluated_presets = preset_bundle.evaluated_presets;
    auto config_it                = preset_bundle.printer_configs.begin();
    while (config_it != preset_bundle.printer_configs.end()) {
        if (evaluated_presets.contains(config_it->second.id)
            && config_it->second.name == selected_printer_name)
            break;
        ++config_it;
    }
    ASSERT(config_it != preset_bundle.printer_configs.end());

    const auto& printer_presets = evaluated_presets.at(config_it->second.id);
    const auto printer_it       = printer_presets.begin();
    ASSERT(printer_it != printer_presets.end());

    fill_config_container_with_selected_preset(cc, config_it->first, printer_it->preset.id);
}

void PresetInteractor::on_selected_config_container_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId container_id
)
{
    m_selected_project_id                                                  = project_id;
    get_or_create_project_context(project_id).selected_config_container_id = container_id;

    // update selected config
    get_or_create_config_container_context(m_selected_project_id, container_id);

    fill_printer_presets();
    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();

    fill_tool_items(selected_preset.hw_config);
    fill_sheet_items(selected_preset.hw_config);

    fill_print_presets(selected_preset);
    fill_tools_presets(selected_preset);
    fill_materials_presets(selected_preset);

    // notify listeners on changes
    invoke_listeners<IPresetChangedListener>(
        [&](auto* l) { l->on_config_container_selection_changed(project_id, container_id); }
    );
    invoke_slicing_input_changed();

    m_printer_cbi.set_config_box(&selected_preset.printer.config_box());
    m_print_cbi.set_config_box(&selected_preset.print.config_box());
}

void PresetInteractor::update_changed_selected_preset_hw_config()
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);
    const auto& hw_config                       = cc->selected_preset().hw_config;
    auto& preset_bundle                         = m_workbench.preset_bundle();
    //FIXME: this will fail if config is runtime one (i.e. loaded from project)
    preset_bundle.printer_configs[hw_config.id] = hw_config;
    auto& vendor_bundle = preset_bundle.vendor_bundles.at(hw_config.vendor_id);
    auto* vb_config     = vendor_bundle.find_printer_config(hw_config.id);
    if (vb_config == nullptr)
        vendor_bundle.printer_configs.push_back(hw_config);
    else
        *vb_config = hw_config;

    PresetEvaluator preset_evaluator{vendor_bundle.presets};
    const std::string& printer_root_id = selected_printer_preset().printer.root_id;
    std::string selected_printer_preset_id;
    try {
        auto& evaluated_printer_presets = preset_bundle.evaluated_presets[hw_config.id];
        evaluated_printer_presets.clear();
        auto epps = preset_evaluator.evaluate(hw_config);
        ASSERT(epps.size() > 0);
        for (auto& epp : epps) {
            //dump_ep_info(epp);
            if (epp.preset.root_id == printer_root_id)
                selected_printer_preset_id = epp.preset.id;

            evaluated_printer_presets.emplace_back(std::move(epp));
        }
        // we didn't find printer with same root_id, let's just select first one
        if (selected_printer_preset_id.empty())
            selected_printer_preset_id = evaluated_printer_presets.front().preset.id;

    } catch (const std::exception& e) {
        SPDLOG_ERROR("{}", e.what());
    }

    fill_printer_presets();
    select_printer_preset(hw_config.id, selected_printer_preset_id);
}

const std::string& PresetInteractor::selected_hw_config_id() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);
    return cc->selected_preset().hw_config.id;
}

const Domain::Preset::HwPrinterConfig& PresetInteractor::current_printer_config() const
{
    return get_printer_config((selected_hw_config_id())).first.get();
}

const Domain::Preset::EvaluatedPrinterPreset::Preset&
PresetInteractor::current_printer_preset() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);

    const auto& selected_preset = cc->selected_preset();
    const auto& hw_config_id    = selected_preset.hw_config.id;
    const auto& printer_id      = selected_preset.printer.id;
    return get_printer_preset(hw_config_id, printer_id).first.get();
}

const Domain::Preset::SelectedPreset& PresetInteractor::selected_printer_preset() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    return cc->selected_preset();
}

PresetInteractorConfigContainerContext&
PresetInteractor::mutable_selected_config_container_context()
{
    auto& project_ctx = get_project_context(m_selected_project_id)->second;
    auto& cccs        = project_ctx.config_containers;
    return cccs.find(project_ctx.selected_config_container_id)->second;
}

Domain::Preset::SelectedPreset& PresetInteractor::mutable_selected_printer_presets()
{
    auto& project   = m_workbench.project(m_selected_project_id);
    const auto& ccc = selected_config_container_context();
    auto* cc        = project.find_config_container(ccc.config_container_id);
    return cc->mutable_selected_preset();
}

void update_hw_config_tools_and_materials_features_from_preset(
    Domain::Preset::SelectedPreset& preset
)
{
    using Domain::overloaded;
    using Domain::Preset::override_features;

    override_features(preset.hw_config.features, preset.printer.features);

    for (size_t i = 0, n = preset.tools.size(); i < n; i++) {
        override_features(preset.hw_config.tools[i].features, preset.printer.features);
        override_features(preset.hw_config.tools[i].features, preset.tools[i].features);
    }

    for (size_t i = 0, n = preset.materials.size(); i < n; i++) {
        const auto& src_mat = preset.materials[i];
        auto& dest_mat =
            preset.hw_config.materials[Domain::Preset::Address{static_cast<uint8_t>(i)}];
        override_features(dest_mat.features, src_mat.features);
        // TODO: maybe we should use '$.type.abbre'
        std::visit(
            overloaded{
                [&dest_mat](const Domain::FilamentSettings& v)
                {
                    auto it = v.find("filament_type");
                    if (it.item)
                        dest_mat.type = it.item->get<std::string>();
                },
                [](const auto& v) {}
            },
            src_mat.values
        );
        dest_mat.id = src_mat.id;
    }
}

template <typename R>
auto get_first_from_range(R&& range)
{
    for (auto it : range)
        return it;
    PANIC("Empty range");
}

void PresetInteractor::fill_config_container_with_selected_preset(
    Domain::ConfigContainer& cc,
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
)
{
    const auto& hw_config      = get_printer_config(printer_hw_config_id).first.get();
    const auto& printer_preset = get_printer_preset(printer_hw_config_id, printer_preset_id).first.get();

    const auto& p = get_or_create_project_context(m_selected_project_id);

    const auto& preset_bundle = m_workbench.preset_bundle();
    PrintPresetProjectView
        print_view(preset_bundle, p.runtime_presets, printer_hw_config_id, printer_preset_id);
    auto print = get_first_from_range(print_view.items()).first;

    std::vector<Domain::Preset::EvaluatedToolPrintPreset::Preset> tools;

    if (hw_config.technology == Domain::PrinterTechnology::FFF) {
        for (size_t n = hw_config.tool_count, tool_idx = 0; tool_idx < n; ++tool_idx) {
            // TODO: better choose tool-print preset + ask for config values transfer
            ToolPrintPresetProjectView tool_view(
                preset_bundle,
                p.runtime_presets,
                printer_hw_config_id,
                printer_preset_id,
                print.get().id,
                tool_idx
            );
            tools.emplace_back(get_first_from_range(tool_view.items()).first.get());
        }
    }

    std::vector<Domain::Preset::EvaluatedMaterialPreset::Preset> materials;
    for (size_t n = hw_config.tool_count, slot_idx = 0; slot_idx < n; ++slot_idx) {
        // TODO: better choose tool-print preset + ask for config values transfer
        MaterialPresetProjectView material_view(
            preset_bundle,
            p.runtime_presets,
            printer_hw_config_id,
            printer_preset_id,
            print.get().id,
            slot_idx
        );
        materials.emplace_back(get_first_from_range(material_view.items()).first.get());
    }

    auto& selected_preset = cc.mutable_selected_preset();
    selected_preset       = Domain::Preset::SelectedPreset{
              .hw_config = hw_config,
              .printer   = printer_preset,
              .print     = print,
              .tools     = std::move(tools),
              .materials = std::move(materials)
    };
    update_hw_config_tools_and_materials_features_from_preset(selected_preset);
}

namespace {

void
set_items(PresetItemObservableList& dest, std::vector<PresetItem>&& items, size_t selected_index)
{
    ASSERT(selected_index != Domain::INVALID_ID);
    dest.items().set_items(std::move(items));
    dest.set_selected_index(selected_index);
}

template <typename V, typename TF, typename TS>
std::optional<size_t> set_items(
    PresetItemObservableList& dest,
    const V& source,
    const std::string& hw_config_id,
    const std::string& hw_config_name,
    const Domain::Preset::EvaluatedPreset<TF, TS>& selected
)
{
    std::optional<std::size_t> selected_index;
    std::optional<std::size_t> selected_by_name_index;
    std::vector<PresetItem> items;
    size_t idx = 0;

    for (const auto& [item_rw, is_runtime] : source.items()) {
        const auto& item = item_rw.get();
        std::string name;
        name = item.short_name();
        items.emplace_back(item.id, std::move(name), hw_config_id, hw_config_name, is_runtime);
        if (item.id == selected.id)
            selected_index = idx;
        if (item.name == selected.short_name())
            selected_by_name_index = idx;
        idx++;
    }

    unsigned long selected_index_val = selected_index.value_or(selected_by_name_index.value_or(0));
    set_items(dest, std::move(items), selected_index_val);
    return selected_index.has_value() ? std::nullopt : std::optional{selected_index_val};
}

} // namespace

void PresetInteractor::fill_printer_presets()
{
    const auto& preset_bundle = m_workbench.preset_bundle();
    const auto& p             = get_or_create_project_context(m_selected_project_id);

    std::vector<PresetItem> items;
    std::optional<size_t> selected_index;
    std::optional<size_t> selected_index_by_hw;
    const auto& selected_preset = selected_printer_preset();
    const HwPrinterConfigProjectView config_view(preset_bundle, p.runtime_presets);
    size_t idx = 0;
    for (const auto& [hw_config_rw, runtime] : config_view.items()) {
        const auto& hw_config = hw_config_rw.get();
        const PrinterPresetProjectView printer_view(preset_bundle, p.runtime_presets, hw_config.id);
        for (const auto& [printer_rw, is_runtime] : printer_view.items()) {
            const auto& printer = printer_rw.get();
            items.emplace_back(printer.id, printer.name, hw_config.id, hw_config.name, is_runtime);
            const auto& item = items.back();
            if (item.hw_printer_config_id == selected_preset.hw_config.id)
            {
                if (item.id == selected_preset.printer.id)
                    selected_index = idx;
                else
                    selected_index_by_hw = idx;
            }

            idx++;
        }
    }
    m_printer_presets.items().set_items(std::move(items));
    size_t selected_index_val = selected_index.value_or(selected_index_by_hw.value_or(0));
    m_printer_presets.set_selected_index(selected_index_val);

    // make sure the new selection is propagated
    if (!selected_index.has_value()) {
        const auto& selected_item = m_printer_presets.items().at(selected_index_val);
        select_printer_preset(selected_item.hw_printer_config_id, selected_item.id);
    }
}

void PresetInteractor::fill_print_presets(Domain::Preset::SelectedPreset& selected_preset)
{
    const auto& p                      = get_or_fail_project_context(m_selected_project_id);
    const auto& hw_config              = selected_preset.hw_config;
    const auto& hw_config_id           = hw_config.id;
    const auto changed_selection_index = set_items(
        m_print_presets,
        PrintPresetProjectView(
            m_workbench.preset_bundle(),
            p.runtime_presets,
            hw_config_id,
            selected_preset.printer.id
        ),
        hw_config_id,
        hw_config.name,
        selected_preset.print
    );
    if (changed_selection_index) {
        const auto& item = m_print_presets.items().at(changed_selection_index.value());
        select_print_preset(item.id);
    }
}

void PresetInteractor::fill_tools_presets(Domain::Preset::SelectedPreset& selected_preset)
{
    const Domain::Preset::HwPrinterConfig& hw_config = selected_preset.hw_config;
    std::vector<PresetItemObservableList> tools;

    const size_t tool_count = hw_config.tool_count;
    ASSERT(
        hw_config.technology != Domain::PrinterTechnology::FFF || tool_count > 0,
        selected_preset.print.id
    );
    const bool has_tool_prints = hw_config.technology == Domain::PrinterTechnology::FFF;
    std::vector<std::optional<size_t>> changed_selected_indices;

    if (has_tool_prints) {
        ASSERT(tool_count == selected_preset.tools.size());

        const auto& p = get_or_fail_project_context(m_selected_project_id);
        for (size_t n = selected_preset.hw_config.tool_count, tool_index = 0; tool_index < n;
             ++tool_index)
        {
            PresetItemObservableList items;
            ToolPrintPresetProjectView view(
                m_workbench.preset_bundle(),
                p.runtime_presets,
                hw_config.id,
                selected_preset.printer.id,
                selected_preset.print.id,
                tool_index
            );

            auto changes_selected_index =
                set_items(items, view, hw_config.id, hw_config.name, selected_preset.tools[tool_index]);

            tools.emplace_back(std::move(items));
            changed_selected_indices.emplace_back(changes_selected_index);
        }
    }

    m_tool_print_presets.set_items(std::move(tools));
    if (has_tool_prints) {
        for (size_t n = selected_preset.hw_config.tool_count, tool_index = 0; tool_index < n;
             ++tool_index)
        {
            const auto changed_selected_index = changed_selected_indices.at(tool_index);
            if (changed_selected_index.has_value()) {
                const auto& item =
                    m_tool_print_presets.at(tool_index).items().at(changed_selected_index.value());
                select_tool_print_preset(tool_index, item.id);
            }
        }
    }

    fill_selected_tool_print_cbis(selected_preset);
}

void PresetInteractor::fill_materials_presets(Domain::Preset::SelectedPreset& selected_preset)
{
    std::vector<PresetItemObservableList> materials;
    std::vector<std::optional<size_t>> changed_selected_indices;
    const auto& p         = get_or_fail_project_context(m_selected_project_id);
    const auto& hw_config = selected_preset.hw_config;
    for (size_t n = hw_config.tool_count, slot_index = 0; slot_index < n; ++slot_index) {
        PresetItemObservableList items;
        MaterialPresetProjectView view(
            m_workbench.preset_bundle(),
            p.runtime_presets,
            hw_config.id,
            selected_preset.printer.id,
            selected_preset.print.id,
            slot_index
        );
        auto changed_selected_index =
            set_items(items, view, hw_config.id, hw_config.name, selected_preset.materials[slot_index]);
        changed_selected_indices.emplace_back(changed_selected_index);
        materials.emplace_back(std::move(items));
    }
    m_material_presets.set_items(std::move(materials));

    fill_selected_material_cbis(selected_preset);

    for (size_t n = hw_config.tool_count, slot_index = 0; slot_index < n; ++slot_index) {
        const auto changed_selected_index = changed_selected_indices.at(slot_index);
        if (changed_selected_index.has_value()) {
            const auto& item =
                m_material_presets.at(slot_index).items().at(changed_selected_index.value());
            select_material_preset(slot_index, item.id);
        }
    }
}

void PresetInteractor::fill_tool_items(const Domain::Preset::HwPrinterConfig& hw_config)
{
    HwConfigEvaluator config_eval;
    const auto& tools = hw_config.vendor_id.empty() ? std::map<std::string, Domain::Preset::HwToolConfigDef>{} : (m_workbench.preset_bundle()
            .vendor_bundles.at(hw_config.vendor_id)
            .vendor_data.defs.at(hw_config.technology)
            .tools);
    HwToolConfigIterator iterator = config_eval.iterate_tools(
        hw_config,
        tools

    );

    std::vector<Domain::Preset::HwToolConfigDef> tool_items;
    for (; iterator != std::end(iterator); ++iterator) {
        tool_items.push_back(*iterator);
    }
    std::vector<ToolConfigItemObservableList> items;
    items.reserve(hw_config.tool_count);
    for (size_t tool_index = 0; tool_index < hw_config.tool_count; ++tool_index) {
        ToolConfigItemObservableList& item = items.emplace_back();
        item.items().set_items(tool_items);

        // Find selected Tool
        const std::string selected_id = hw_config.tools.at(tool_index).id;
        size_t selected_index         = 0;
        for (HwToolConfigIterator selected_tool_iterator = iterator.begin();
             selected_tool_iterator != iterator.end();
             ++selected_tool_iterator)
        {
            if (selected_id == selected_tool_iterator->id) {
                selected_index = std::distance(iterator.begin(), selected_tool_iterator);
                break;
            }
        }

        item.set_selected_index(selected_index);
    }
    m_tool_items.set_items(std::move(items));
}

void PresetInteractor::fill_sheet_items(const Domain::Preset::HwPrinterConfig& hw_config)
{
    HwConfigEvaluator config_eval;
    HwSheetConfigIterator iterator = config_eval.iterate_sheets(
        hw_config,
        m_workbench.preset_bundle()
            .vendor_bundles.at(hw_config.vendor_id)
            .vendor_data.defs.at(hw_config.technology)
            .sheets
    );

    size_t selected_index = 0;
    std::vector<Domain::Preset::HwSheetConfigDef> items;
    for (; iterator != std::end(iterator); ++iterator) {
        items.push_back(*iterator);
        if (iterator->id == hw_config.sheet.id) {
            selected_index = std::distance(std::begin(iterator), iterator);
        }
    }
    m_sheet_items.items().set_items(items);
    if (!items.empty()) { // SLA doesn't yet have sheets specified
        m_sheet_items.set_selected_index(selected_index);
    }
}

void PresetInteractor::fill_selected_tool_print_cbis(Domain::Preset::SelectedPreset& selected_preset)
{
    // Remove previous tool accessors
    for (size_t tool_index = 0; tool_index < m_tool_cbi_list.size(); ++tool_index) {
        m_cbi_accessors.erase(&m_tool_cbi_list.at(tool_index));
    }

    std::vector<Domain::ConfigBox*> tool_cbs;
    tool_cbs.reserve(selected_preset.tools.size());
    std::transform(
        selected_preset.tools.begin(),
        selected_preset.tools.end(),
        std::back_inserter(tool_cbs),
        [](Domain::Preset::EvaluatedToolPrintPreset::Preset& preset)
        { return &preset.config_box(); }
    );

    SetAccessorMap accessors = m_tool_cbi_list.set_items(tool_cbs);
    m_cbi_accessors.insert(accessors.begin(), accessors.end());
}

void PresetInteractor::fill_selected_material_cbis(Domain::Preset::SelectedPreset& selected_preset)
{
    // Remove previous material accessors
    for (size_t material_index = 0; material_index < m_material_cbi_list.size(); ++material_index) {
        m_cbi_accessors.erase(&m_material_cbi_list.at(material_index));
    }

    std::vector<Domain::ConfigBox*> material_cbs;
    material_cbs.reserve(selected_preset.materials.size());
    std::transform(
        selected_preset.materials.begin(),
        selected_preset.materials.end(),
        std::back_inserter(material_cbs),
        [](Domain::Preset::EvaluatedMaterialPreset::Preset& preset) { return &preset.config_box(); }
    );

    SetAccessorMap accessors = m_material_cbi_list.set_items(material_cbs);
    m_cbi_accessors.insert(accessors.begin(), accessors.end());

}


void PresetInteractor::select_printer_preset(
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
)
{
    auto& project   = m_workbench.project(m_selected_project_id);
    const auto& ccc = selected_config_container_context();
    auto* cc        = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr, ccc.config_container_id);

    fill_config_container_with_selected_preset(*cc, printer_hw_config_id, printer_preset_id);

    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();

    fill_tool_items(selected_preset.hw_config);
    fill_sheet_items(selected_preset.hw_config);

    fill_print_presets(selected_preset);

    m_printer_presets.set_selected(
        [&printer_hw_config_id, &printer_preset_id](const PresetItem& item)
        {
            return item.id == printer_preset_id
                && item.hw_printer_config_id == printer_hw_config_id;
        }
    );
    m_printer_cbi.set_config_box(&selected_preset.printer.config_box());
    m_print_cbi.set_config_box(&selected_preset.print.config_box());

    fill_tools_presets(selected_preset);
    fill_materials_presets(selected_preset);


    // notify on change
    invoke_listeners<IPresetChangedListener>(
        [project_id = m_selected_project_id, &cc](auto* l)
        { l->on_preset_selection_changed(project_id, cc->id().id, PresetItemType::PrinterPreset); }
    );
    invoke_slicing_input_changed();
}

void PresetInteractor::select_print_preset(const std::string& id)
{
    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();
    const auto& hw_config                           = selected_preset.hw_config;
    const auto& printer = get_printer_preset(hw_config.id, selected_preset.printer.id).first.get();

    const auto& p = get_print_preset(hw_config.id, printer.id, id).first.get();

    selected_preset.print = p;

    m_print_presets.set_selected([&id](const PresetItem& item) { return item.id == id; });
    m_print_cbi.set_config_box(&selected_preset.print.config_box());

    fill_tools_presets(selected_preset);
    fill_materials_presets(selected_preset);

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>(
        [project_id = m_selected_project_id, config_container_id](auto* l)
        {
            l->on_preset_selection_changed(
                project_id,
                config_container_id,
                PresetItemType::PrintPreset
            );
        }
    );
    invoke_slicing_input_changed();
}

void PresetInteractor::select_tool_print_preset(size_t tool_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto& t         = get_tool_print_preset(
        selected_preset.hw_config.id,
        selected_preset.printer.id,
        selected_preset.print.id,
        tool_index,
        id
    ).first.get();
    selected_preset.tools[tool_index] = t;

    m_tool_print_presets_writer.mutate_at(
        tool_index,
        [&id](auto& item)
        { item.set_selected([&id](const PresetItem& item) { return item.id == id; }); }
    );

    m_cbi_accessors.at(&m_tool_cbi_list.at(tool_index))
        .set_config_box(&selected_preset.tools.at(tool_index).config_box());


    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>(
        [project_id = m_selected_project_id, config_container_id](auto* l)
        {
            l->on_preset_selection_changed(
                project_id,
                config_container_id,
                PresetItemType::ToolPrintPreset
            );
        }
    );
    invoke_slicing_input_changed();
}

void PresetInteractor::select_material_preset(size_t material_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto& m         = get_material_preset(
        selected_preset.hw_config.id,
        selected_preset.printer.id,
        selected_preset.print.id,
        material_index,
        id
    ).first.get();

    selected_preset.materials[material_index] = m;
    update_hw_config_tools_and_materials_features_from_preset(selected_preset);

    m_material_presets_writer.mutate_at(
        material_index,
        [&id](auto& item)
        { item.set_selected([&id](const PresetItem& item) { return item.id == id; }); }
    );

    m_cbi_accessors.at(&m_material_cbi_list.at(material_index))
        .set_config_box(&selected_preset.materials.at(material_index).config_box());

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>(
        [project_id = m_selected_project_id, config_container_id](auto* l)
        {
            l->on_preset_selection_changed(
                project_id,
                config_container_id,
                PresetItemType::MaterialPreset
            );
        }
    );
    invoke_slicing_input_changed();
}

void PresetInteractor::duplicate_hw_config_if_is_runtime(
    Domain::Preset::SelectedPreset& selected_preset
)
{
    const auto& p = get_or_create_project_context(m_selected_project_id);
    const bool is_runtime_config =
        p.runtime_presets.printer_configs.contains(selected_preset.hw_config.id);
    if (is_runtime_config) {
        auto new_config           = selected_preset.hw_config;
        new_config.id             = generate_uuid();
        selected_preset.hw_config = new_config;
    }
}

void PresetInteractor::select_printer_tool_item(size_t tool_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    duplicate_hw_config_if_is_runtime(selected_preset);

    auto& hw_config = selected_preset.hw_config;
    const auto& vendor_data =
        m_workbench.preset_bundle().vendor_bundles.at(hw_config.vendor_id).vendor_data;
    const auto* tool_def = vendor_data.find_tool_config_def_by_id(id);
    ASSERT(tool_def != nullptr, id);
    hw_config.tools.at(tool_index) = from_def(vendor_data, *tool_def);
    hw_config.name                 = Domain::Preset::suggest_name(hw_config, vendor_data);
    update_changed_selected_preset_hw_config();

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id,
                                              config_container_id](auto* l) {
        l->on_hw_item_selection_changed(project_id, config_container_id, HwItemType::ToolItem);
    });
}

void PresetInteractor::select_printer_sheet(const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    duplicate_hw_config_if_is_runtime(selected_preset);

    const auto& vendor_data = m_workbench.preset_bundle()
                                  .vendor_bundles.at(selected_preset.hw_config.vendor_id)
                                  .vendor_data;
    const auto* sheet_def = vendor_data.find_sheet_config_def_by_id(id);
    ASSERT(sheet_def != nullptr, id);
    selected_preset.hw_config.sheet = from_def(vendor_data, *sheet_def);
    update_changed_selected_preset_hw_config();

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id,
                                              config_container_id](auto* l) {
        l->on_hw_item_selection_changed(project_id, config_container_id, HwItemType::SheetItem);
    });
}

void PresetInteractor::set_preset_value(
    Domain::ConfigLocation location,
    int element_idx,
    const std::string& name,
    ConfigItemModifyFn modify_fn
)
{
    auto& selected_preset = mutable_selected_printer_presets();
    std::optional<Domain::ConfigBox*> config_box_opt;
    if (selected_preset.technology() == Domain::PrinterTechnology::FFF) {
        auto loc = std::get<Domain::FDMConfigLocation>(location);

        switch (loc) {
        case Domain::FDMConfigLocation::Printer:
            config_box_opt = &std::get<Domain::PrinterSettings>(selected_preset.printer.values);
            break;

        case Domain::FDMConfigLocation::Print:
            config_box_opt = &std::get<Domain::PrintSettings>(selected_preset.print.values);
            break;

        case Domain::FDMConfigLocation::Tool:
            config_box_opt =
                &std::get<Domain::ToolPrintSettings>(selected_preset.tools[element_idx].values);
            break;

        case Domain::FDMConfigLocation::Filament:
            config_box_opt =
                &std::get<Domain::FilamentSettings>(selected_preset.materials[element_idx].values);
            break;

        default:
            PANIC("Unsupported location {}", loc);
        }
    } else {
        auto loc = std::get<Domain::SLAConfigLocation>(location);
        switch (loc) {
        case Domain::SLAConfigLocation::Printer:
            config_box_opt = &std::get<Domain::SLAPrinterSettings>(selected_preset.printer.values);
            break;

        case Domain::SLAConfigLocation::Print:
            config_box_opt = &std::get<Domain::SLAPrintSettings>(selected_preset.print.values);
            break;

        case Domain::SLAConfigLocation::Material:
            config_box_opt = &std::get<Domain::SLAMaterialSettings>(
                selected_preset.materials[element_idx].values
            );
            break;

        default:
            PANIC("Unsupported location {}", loc);
        }
    }

    ASSERT(config_box_opt.has_value());
    auto& config_box = *config_box_opt.value();
    auto it          = config_box.find(name);
    ASSERT(it.item != nullptr);
    modify_fn(*it.item);

    invoke_on_preset_value_changed(*it.item);
}

ConfigBoxInteractor& PresetInteractor::printer_cbi()
{
    return m_printer_cbi;
}

ConfigBoxInteractor& PresetInteractor::print_cbi()
{
    return m_print_cbi;
}

CBIObservableList& PresetInteractor::material_cbi_list()
{
    return m_material_cbi_list;
}

CBIObservableList& PresetInteractor::tool_cbi_list()
{
    return m_tool_cbi_list;
}

const Domain::ConfigValue* PresetInteractor::get_override_origin(
    const Domain::ConfigItem& item,
    size_t index
) const
{
    return std::visit([=, this](auto&& location) -> const Domain::ConfigValue* {
        using T = std::decay_t<decltype(location)>;

        const std::string& name = item.name();

        if constexpr (std::is_same_v<T, Domain::FDMConfigLocation>) {
            switch (location) {
            case Domain::FDMConfigLocation::Printer:
                return m_printer_cbi.find(name);
            case Domain::FDMConfigLocation::Print:
                return m_print_cbi.find(name);
            case Domain::FDMConfigLocation::Filament:
                return m_material_cbi_list.at(index).find(name);
            case Domain::FDMConfigLocation::Tool:
                return m_tool_cbi_list.at(index).find(name);
            default:
                break;
            }
        } else if constexpr (std::is_same_v<T, Domain::SLAConfigLocation>) {
            switch (location) {
            case Domain::SLAConfigLocation::Printer:
                return m_printer_cbi.find(name);
            case Domain::SLAConfigLocation::Print:
                return m_print_cbi.find(name);
            case Domain::SLAConfigLocation::Material:
                return m_material_cbi_list.at(index).find(name);
            default:
                break;
            }
        }

        return nullptr;
    }, item.def().location);
}

void PresetInteractor::set_item_value(
    const Domain::ConfigItem& item,
    const Domain::ConfigValue& value,
    size_t index
)
{
    // This is a temporary dummy way how to set items, a whole dependency resolving with overrides needs
    // to be ported here from the Legacy code

    std::visit(
        [=, this](auto&& location)
        {
            using T = std::decay_t<decltype(location)>;

            const std::string& name = item.name();

            if constexpr (std::is_same_v<T, Domain::FDMConfigLocation>) {
                switch (location) {
                case Domain::FDMConfigLocation::Printer:
                    m_cbi_accessors.at(&m_printer_cbi).set_value(name, value);
                    break;
                case Domain::FDMConfigLocation::Print:
                    m_cbi_accessors.at(&m_print_cbi).set_value(name, value);
                    break;
                case Domain::FDMConfigLocation::Filament: {
                    m_cbi_accessors.at(&m_material_cbi_list.at(index)).set_value(name, value);
                } break;
                case Domain::FDMConfigLocation::Tool: {
                    m_cbi_accessors.at(&m_tool_cbi_list.at(index)).set_value(name, value);
                } break;
                case Domain::FDMConfigLocation::Object:
                case Domain::FDMConfigLocation::Volume: {
                    m_object_settings_interactor_accessor.set_value(name, value);
                } break;
                default:
                    break;
                }
            } else if constexpr (std::is_same_v<T, Domain::SLAConfigLocation>) {
                switch (location) {
                case Domain::SLAConfigLocation::Printer:
                    m_cbi_accessors.at(&m_printer_cbi).set_value(name, value);
                    break;
                case Domain::SLAConfigLocation::Print:
                    m_cbi_accessors.at(&m_print_cbi).set_value(name, value);
                    break;
                case Domain::SLAConfigLocation::Material: {
                    m_cbi_accessors.at(&m_material_cbi_list.at(index)).set_value(name, value);
                } break;
                case Domain::SLAConfigLocation::Object: {
                    m_object_settings_interactor_accessor.set_value(name, value);
                } break;
                default:
                    break;
                }
            }
        },
        item.location()
    );
    invoke_on_preset_value_changed(item);
    invoke_slicing_input_changed();
}

void PresetInteractor::set_item_override(const Domain::ConfigItem& item, bool enable, size_t index)
{
    // This is a temporary dummy way how to set items, a whole dependency resolving with overrides needs
    // to be ported here from the Legacy code

    std::visit([=, this](auto&& location) {
        using T = std::decay_t<decltype(location)>;

        const std::string& name = item.name();

        if constexpr (std::is_same_v<T, Domain::FDMConfigLocation>) {
            switch (location) {
            case Domain::FDMConfigLocation::Printer:
                m_cbi_accessors.at(&m_printer_cbi).set_override(name, enable);
                break;
            case Domain::FDMConfigLocation::Print:
                m_cbi_accessors.at(&m_print_cbi).set_override(name, enable);
                break;
            case Domain::FDMConfigLocation::Filament: {
                m_cbi_accessors.at(&m_material_cbi_list.at(index)).set_override(name, enable);
            } break;
            case Domain::FDMConfigLocation::Tool: {
                m_cbi_accessors.at(&m_tool_cbi_list.at(index)).set_override(name, enable);
            } break;
            case Domain::FDMConfigLocation::Object:
            case Domain::FDMConfigLocation::Volume: {
                m_object_settings_interactor_accessor.set_override(name, enable);
            } break;
            default:
                break;
            }
        } else if constexpr (std::is_same_v<T, Domain::SLAConfigLocation>) {
            switch (location) {
            case Domain::SLAConfigLocation::Printer:
                m_cbi_accessors.at(&m_printer_cbi).set_override(name, enable);
                break;
            case Domain::SLAConfigLocation::Print:
                m_cbi_accessors.at(&m_print_cbi).set_override(name, enable);
                break;
            case Domain::SLAConfigLocation::Material: {
                m_cbi_accessors.at(&m_material_cbi_list.at(index)).set_override(name, enable);
            } break;
            case Domain::SLAConfigLocation::Object: {
                m_object_settings_interactor_accessor.set_override(name, enable);
            } break;
            default:
                break;
            }
        }
    }, item.location());
    invoke_on_preset_value_changed(item);
    invoke_slicing_input_changed();
}

void PresetInteractor::invoke_slicing_input_changed()
{
    const auto& ccc     = selected_config_container_context();
    const auto& project = m_workbench.project(m_selected_project_id);
    for (const auto& instance :
         project.find_config_container(ccc.config_container_id)->bed_instances())
    {
        invoke_listeners<ISlicingInputChangedListener>(
            [&](auto listener)
            { listener->on_slicing_input_changed({ccc.config_container_id, instance->id().id}); }
        );
    }
}

void PresetInteractor::invoke_on_preset_value_changed(const Domain::ConfigItem& config_item)
{
    const auto& ccc = selected_config_container_context();
    invoke_listeners<IPresetChangedListener>(
        [&](auto listener)
        {
            listener->on_preset_value_changed(
                m_selected_project_id,
                ccc.config_container_id,
                config_item
            );
        }
    );
}

PresetInteractorProjectContext& PresetInteractor::get_or_create_project_context(
    Domain::SelectionId project_id
)
{
    auto it = m_project_contexts.find(project_id);
    if (it != m_project_contexts.end())
        return it->second;

    bool _;
    std::tie(it, _) =
        m_project_contexts.emplace(project_id, PresetInteractorProjectContext{project_id});

    return it->second;
}

PresetInteractorConfigContainerContext& PresetInteractor::get_or_create_config_container_context(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    auto& project         = m_workbench.project(project_id);
    auto& project_context = get_or_create_project_context(project_id);
    auto it               = project_context.config_containers.find(config_container_id);
    if (it != project_context.config_containers.end())
        return it->second;

    PresetInteractorConfigContainerContext ccc{config_container_id};
    Domain::ConfigContainer& cc = *ASSERT_VAL(project.find_config_container(config_container_id));

    // TODO: initialize ConfigContainerContext here

    bool _;
    std::tie(it, _) =
        project_context.config_containers.emplace(config_container_id, std::move(ccc));
    return it->second;
}

namespace {
using Domain::Preset::HwPrinterConfig;
using Domain::Preset::HwPrinterConfigs;

const HwPrinterConfig*
find_same_config(const HwPrinterConfigs& bundle_configs, const HwPrinterConfig& hw_config)
{
    auto it = std::ranges::find_if(
        bundle_configs,
        [&hw_config](const HwPrinterConfig& cfg) { return hw_config.has_same_values(cfg); }
    );
    return &*it;
}

template <typename T>
concept HavingPresetWithId = requires(T a) {
    a.preset.id;

    requires std::same_as<std::string, std::remove_cvref_t<decltype(a.preset.id)>>;
};

template <HavingPresetWithId T>
const T* find_by_id(const std::vector<T>& data, const std::string& id)
{
    auto it =
        std::find_if(data.begin(), data.end(), [&id](const T& x) { return x.preset.id == id; });
    return it != data.end() ? &*it : nullptr;
}

} // namespace

void append_evaluated_presets_to_runtime(
    RuntimePresets& runtime_presets,
    const HwPrinterConfig& hw_config,
    const PresetEvaluator::EvaluatedPrinterPresets& epps
)
{
    auto& dest_printers = runtime_presets.printer[hw_config.id];
    for (const auto& epp : epps) {
        if (runtime_presets.find_printer_preset_by_id(hw_config.id, epp.preset.id) != nullptr)
            continue;
        dest_printers.push_back(epp.preset);

        const RuntimePresets::HwConfigPrinterKey printer_key{hw_config.id, epp.preset.id};

        for (const auto& p : epp.prints) {
            if (runtime_presets.find_print_preset_by_id(printer_key, p.preset.id) != nullptr)
                continue;
            runtime_presets.print[printer_key].push_back(p.preset);

            const RuntimePresets::HwConfingPrinterPrintKey print_key{
                hw_config.id,
                epp.preset.id,
                p.preset.id
            };

            size_t tool_idx = 0;
            for (const auto& tools : p.tools) {
                for (const auto& tool : tools) {
                    if (runtime_presets
                            .find_tool_print_preset_by_id(print_key, tool_idx, tool.preset.id)
                        != nullptr)
                        continue;
                    ;
                    runtime_presets.add_tool_print(print_key, hw_config, tool_idx, tool.preset);
                }
                tool_idx++;
            }

            size_t material_idx = 0;
            for (const auto& materials : p.materials) {
                for (const auto& material : materials) {
                    if (runtime_presets
                            .find_material_preset_by_id(print_key, material_idx, material.preset.id)
                        != nullptr)
                        continue;
                    runtime_presets
                        .add_material(print_key, hw_config, material_idx, material.preset);
                }
                material_idx++;
            }
        }
    }
}

void PresetInteractor::load_selected_preset_from_3mf(
    Domain::SelectionId project_id,
    Domain::Preset::SelectedPreset& selected_preset
)
{
    auto& pc              = get_or_create_project_context(project_id);
    auto& runtime_presets = pc.runtime_presets;

    bool runtime_presets_evaluation_required = false;
    const auto& preset_bundle = m_workbench.preset_bundle();
    // 1. Reconcile HwPrinterConfig
    // deduplicate existing hw configuration (same_values())
    const auto printer_configs_view = get_printer_configs_view(project_id);
    const auto* same_hw_config =
        printer_configs_view.find_with_same_values(selected_preset.hw_config);
    if (same_hw_config != nullptr) {
        selected_preset.hw_config = *same_hw_config;
    } else {
        // check for duplicate id, if conflict update to new ID and update selected_preset with that ID
        if (printer_configs_view.has_conflicting_id(selected_preset.hw_config.id))
            selected_preset.hw_config.id = generate_uuid();
        runtime_presets.printer_configs.emplace(
            std::pair{selected_preset.hw_config.id, selected_preset.hw_config}
        );
        runtime_presets_evaluation_required = true;
    }

    const auto& config_id = selected_preset.hw_config.id;

    // 2. Reconcile PrinterPreset
    // deduplicate existing printer preset (check for config box differences)
    const auto printer_presets_view = get_printer_presets_view(project_id, config_id);
    const Domain::Preset::EvaluatedPrinterPreset::Preset* ep_printer =
        printer_presets_view
            .find_with_same_values(selected_preset.printer);

    if (ep_printer != nullptr) {
        selected_preset.printer = *ep_printer;
    } else {
        // check for duplicate id, if conflict update to new ID and update selected_preset with that ID
        if (printer_presets_view.has_conflicting_id(selected_preset.printer.id)) {
            selected_preset.printer.id = generate_uuid();
        }
        runtime_presets.printer[config_id].push_back(selected_preset.printer);
    }
    const auto& printer_id = selected_preset.printer.id;

    // 3. Reconcile PrintPreset
    // deduplicate existing print preset (check for config box differences)
    const auto print_presets_view = get_print_presets_view(project_id, config_id, printer_id);
    const auto* ep_print = ep_printer == nullptr ?
        nullptr :
        print_presets_view.find_with_same_values(selected_preset.print);
    if (ep_print != nullptr) {
        selected_preset.print = *ep_print;
    } else {
        // check for duplicate id, if conflict update to new ID and update selected_preset with that ID
        if (print_presets_view.has_conflicting_id(selected_preset.print.id)) {
            selected_preset.print.id = generate_uuid();
        }
        const RuntimePresets::HwConfigPrinterKey printer_key{config_id, printer_id};
        runtime_presets.print[printer_key].push_back(selected_preset.print);
    }
    const auto& print_id = selected_preset.print.id;

    // 4. Reconcile ToolPrintPreset
    const RuntimePresets::HwConfingPrinterPrintKey printer_print_key{
        config_id,
        printer_id,
        print_id
    };

    if (selected_preset.hw_config.technology == Domain::PrinterTechnology::FFF) {
        for (size_t tool_index = 0; tool_index < selected_preset.hw_config.tool_count; ++tool_index) {
            auto& selected_tool_print = selected_preset.tools.at(tool_index);
            // deduplicate existing tool_print preset (check for config box differences)
            const auto tool_print_presets_view = get_tool_print_presets_view(
                project_id,
                config_id,
                printer_id,
                print_id,
                tool_index
            );
            const auto* ep_tool_print = ep_print == nullptr ?
                nullptr :
                tool_print_presets_view.find_with_same_values(selected_tool_print);

            if (ep_tool_print != nullptr) {
                selected_tool_print = *ep_tool_print;
            } else {
                // check for duplicate id, if conflict update to new ID and update selected_preset with that ID
                if (tool_print_presets_view.has_conflicting_id(selected_tool_print.id)) {
                    selected_tool_print.id = generate_uuid();
                }
                runtime_presets.add_tool_print(
                    printer_print_key,
                    selected_preset.hw_config,
                    tool_index,
                    selected_tool_print
                );
            }
        }
    }

    // 5. Reconcile ToolPrintPreset
    for (size_t slot_index = 0; slot_index < selected_preset.hw_config.tool_count; ++slot_index) {
        auto& selected_material = selected_preset.materials.at(slot_index);

        // deduplicate existing tool_print preset (check for config box differences)
        const auto material_presets_view = get_material_presets_view(
            project_id,
            config_id,
            printer_id,
            print_id,
            slot_index
        );
        const auto* ep_material = ep_print == nullptr ?
            nullptr :
            material_presets_view.find_with_same_values(selected_material);
        if (ep_material != nullptr) {
            selected_material = *ep_material;
        } else {
            // check for duplicate id, if conflict update to new ID and update selected_preset with that ID
            if (material_presets_view.has_conflicting_id(selected_material.id)) {
                selected_material.id = generate_uuid();
            }
            runtime_presets.add_material(
                printer_print_key,
                selected_preset.hw_config,
                slot_index,
                selected_material
            );
        }
    }

    // 6. Insert evaluated presets to runtime
    if (runtime_presets_evaluation_required) {
        auto vb_it = preset_bundle.vendor_bundles.find(selected_preset.hw_config.vendor_id);
        if (vb_it != preset_bundle.vendor_bundles.end()) {
            const auto& vendor_bundle = vb_it->second;
            PresetEvaluator preset_evaluator{vendor_bundle.presets};
            try {
                auto epps             = preset_evaluator.evaluate(selected_preset.hw_config);
                const auto& hw_config = selected_preset.hw_config;
                append_evaluated_presets_to_runtime(runtime_presets, hw_config, epps);
            } catch (const std::exception& e) {
                SPDLOG_ERROR("{}", e.what());
            }
        }
    }
}

PresetInteractor::ConstRefBoolPair<Domain::Preset::HwPrinterConfig>
PresetInteractor::get_printer_config(
    Domain::SelectionId project_id,
    const std::string& hw_config_id
) const
{
    const auto& printer_configs = m_workbench.preset_bundle().printer_configs;
    if (auto it = printer_configs.find(hw_config_id); it != printer_configs.end())
        return {std::cref(it->second), false};
    const auto& p   = get_or_fail_project_context(project_id);
    const auto* cfg = p.runtime_presets.find_printer_config_by_id(hw_config_id);

    ASSERT(cfg != nullptr, hw_config_id);
    return {std::cref(*cfg), true};
}

PresetInteractor::ConstRefBoolPair<Domain::Preset::EvaluatedPrinterPreset::Preset>
PresetInteractor::get_printer_preset(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id
) const
{
    const auto* printer_preset =
        m_workbench.preset_bundle().find_printer_preset(hw_config_id, printer_preset_id);
    if (printer_preset != nullptr)
        return {std::cref(printer_preset->preset), false};

    const auto& p = get_or_fail_project_context(project_id);
    const auto* runtime_printer_preset =
        p.runtime_presets.find_printer_preset_by_id(hw_config_id, printer_preset_id);
    ASSERT(runtime_printer_preset != nullptr, std::tuple{hw_config_id, printer_preset_id});
    return {std::cref(*runtime_printer_preset), true};
}

PresetInteractor::ConstRefBoolPair<Domain::Preset::EvaluatedPrintPreset::Preset>
PresetInteractor::get_print_preset(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id,
    const std::string& print_id
) const
{
    const auto* printer_preset =
        m_workbench.preset_bundle().find_printer_preset(hw_config_id, printer_preset_id);
    if (printer_preset != nullptr) {
        const auto* print = find_by_id(printer_preset->prints, print_id);
        if (print != nullptr)
            return {std::cref(print->preset), false};
    }
    const auto& p = get_or_fail_project_context(project_id);
    const auto* print =
        p.runtime_presets.find_print_preset_by_id({hw_config_id, printer_preset_id}, print_id);
    ASSERT(print != nullptr, std::tuple{hw_config_id, printer_preset_id, print_id});
    return {std::cref(*print), true};
}

PresetInteractor::ConstRefBoolPair<Domain::Preset::EvaluatedToolPrintPreset::Preset> PresetInteractor::get_tool_print_preset(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id,
    const std::string& print_preset_id,
    size_t tool_index,
    const std::string& tool_print_preset_id
) const
{
    const auto* printer_preset =
        m_workbench.preset_bundle().find_printer_preset(hw_config_id, printer_preset_id);
    if (printer_preset != nullptr) {
        const auto* print = find_by_id(printer_preset->prints, print_preset_id);
        if (print != nullptr) {
            const auto* tool_print = find_by_id(print->tools[tool_index], tool_print_preset_id);
            if (tool_print != nullptr) {
                return {std::cref(tool_print->preset), false};
            }
        }
    }
    const auto& p          = get_or_fail_project_context(project_id);
    const auto* tool_print = p.runtime_presets.find_tool_print_preset_by_id(
        {hw_config_id, printer_preset_id, print_preset_id},
        tool_index,
        tool_print_preset_id
    );
    ASSERT(
        tool_print != nullptr,
        std::tuple{hw_config_id, printer_preset_id, print_preset_id, tool_print_preset_id}
    );
    return {std::cref(*tool_print), true};
}

PresetInteractor::ConstRefBoolPair<Domain::Preset::EvaluatedMaterialPreset::Preset>
PresetInteractor::get_material_preset(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id,
    const std::string& print_preset_id,
    size_t slot_index,
    const std::string& material_preset_id
) const
{
    const auto* printer_preset =
        m_workbench.preset_bundle().find_printer_preset(hw_config_id, printer_preset_id);
    if (printer_preset != nullptr) {
        const auto* print = find_by_id(printer_preset->prints, print_preset_id);
        if (print != nullptr) {
            const auto* material = find_by_id(print->materials[slot_index], material_preset_id);
            if (material != nullptr) {
                return {std::cref(material->preset), false};
            }
        }
    }
    const auto& p        = get_or_fail_project_context(project_id);
    const auto* material = p.runtime_presets.find_material_preset_by_id(
        {hw_config_id, printer_preset_id, print_preset_id},
        slot_index,
        material_preset_id
    );
    ASSERT(
        material != nullptr,
        std::tuple{hw_config_id, printer_preset_id, print_preset_id, material_preset_id}
    );
    return {std::cref(*material), true};
}

HwPrinterConfigProjectView PresetInteractor::get_printer_configs_view(
    Domain::SelectionId project_id
) const
{
    return {
        m_workbench.preset_bundle(),
        get_or_fail_project_context(project_id).runtime_presets
    };
}

[[nodiscard]] PrinterPresetProjectView PresetInteractor::get_printer_presets_view(
    Domain::SelectionId project_id,
    const std::string& hw_config_id
) const
{
    return {
        m_workbench.preset_bundle(),
        get_or_fail_project_context(project_id).runtime_presets,
        hw_config_id
    };
}

PrintPresetProjectView PresetInteractor::get_print_presets_view(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id
) const
{
    return {
        m_workbench.preset_bundle(),
        get_or_fail_project_context(project_id).runtime_presets,
        hw_config_id,
        printer_preset_id
    };
}

ToolPrintPresetProjectView PresetInteractor::get_tool_print_presets_view(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id,
    const std::string& print_preset_id,
    size_t tool_index
) const
{
    return {
        m_workbench.preset_bundle(),
        get_or_fail_project_context(project_id).runtime_presets,
        hw_config_id,
        printer_preset_id,
        print_preset_id,
        tool_index
    };
}

MaterialPresetProjectView PresetInteractor::get_material_presets_view(
    Domain::SelectionId project_id,
    const std::string& hw_config_id,
    const std::string& printer_preset_id,
    const std::string& print_preset_id,
    size_t slot_index
) const
{
    return {
        m_workbench.preset_bundle(),
        get_or_fail_project_context(project_id).runtime_presets,
        hw_config_id,
        printer_preset_id,
        print_preset_id,
        slot_index
    };
}

} // namespace Slic3r::Biz::Preset
