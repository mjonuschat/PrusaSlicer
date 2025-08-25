#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Preset/HwConfigEvaluator.hpp"
#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/IO/BundleLoader.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"

#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
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

PresetInteractor::PresetInteractor(Domain::Workbench& workbench) : m_workbench(workbench)
{
    ConfigBoxInteractor::SetAccessor printer_accessor;
    m_printer_cbi                   = ConfigBoxInteractor(printer_accessor, nullptr);
    m_cbi_accessors[&m_printer_cbi] = std::move(printer_accessor);

    ConfigBoxInteractor::SetAccessor print_accessor;
    m_print_cbi                   = ConfigBoxInteractor(print_accessor, nullptr);
    m_cbi_accessors[&m_print_cbi] = std::move(print_accessor);
}

void PresetInteractor::load_preset_bundle(const std::string& preset_bundle_path, const std::string& config_path)
{
    auto preset_bundle = IO::load_bundle(preset_bundle_path, config_path);

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
    for (const auto& [vendor_id, vendor_bundle] : preset_bundle.vendor_bundles) {
        PresetEvaluator preset_evaluator{vendor_bundle.presets};
        for (const auto& p : vendor_bundle.printer_configs) {
            try {
                auto epps = preset_evaluator.evaluate(p);
                for (auto& epp : epps) {
                    dump_ep_info(epp);
                    preset_bundle.evaluated_presets[epp.hw_config.id].emplace_back(std::move(epp));
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("{}", e.what());
            }
        }
    }
    // do not save it now, as we create it anyway again
    // IO::save_bundle_configs(preset_bundle, config_path);

    m_workbench.set_preset_bundle(std::move(preset_bundle));
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

const PresetInteractorConfigContainerContext& PresetInteractor::selected_config_container_context() const
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
    const static std::string selected_printer_name = "CORE One 0.4 HF"; //"SL1S SPEED";//"Prusa MK4S";
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
    auto& ccc = get_or_create_config_container_context(m_selected_project_id, container_id);

    fill_printer_presets();
    const Domain::Preset::EvaluatedPrinterPreset& printer_preset = current_printer_preset();
    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();

    const auto* print = printer_preset.find_print_preset_by_id(selected_preset.print.id);
    ASSERT(print != nullptr, selected_preset.print.id);

    fill_print_presets(printer_preset, selected_preset);
    fill_tools_presets(printer_preset, *print, selected_preset);
    fill_materials_presets(*print, selected_preset);

    fill_tool_items(printer_preset);
    fill_sheet_items(printer_preset);

    // notify listeners on changes
    invoke_listeners<IPresetChangedListener>([&](auto* l) {
        l->on_config_container_selection_changed(project_id, container_id);
    });
    invoke_slicing_input_changed();

    m_printer_cbi.set_config_box(&selected_preset.printer.config_box());
    m_print_cbi.set_config_box(&selected_preset.print.config_box());
}

const std::string& PresetInteractor::selected_hw_config_id() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);
    return cc->selected_preset().hw_config.id;
}

const Domain::Preset::EvaluatedPrinterPreset& PresetInteractor::current_printer_preset() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr);

    const auto& selected_preset = cc->selected_preset();
    const auto& hw_config_id    = selected_preset.hw_config.id;
    const auto& printer_id      = selected_preset.printer.id;
    const auto& printer_presets = m_workbench.preset_bundle().evaluated_presets.at(hw_config_id);
    auto it = std::ranges::find_if(printer_presets, [printer_id](const auto& preset) {
        return preset.preset.id == printer_id;
    });

    ASSERT(it != printer_presets.end());
    return *it;
}

const Domain::Preset::SelectedPreset& PresetInteractor::selected_printer_preset() const
{
    const auto& project = m_workbench.project(m_selected_project_id);
    const auto& ccc     = selected_config_container_context();
    const auto* cc      = project.find_config_container(ccc.config_container_id);
    return cc->selected_preset();
}

PresetInteractorConfigContainerContext& PresetInteractor::mutable_selected_config_container_context()
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

void update_hw_config_tools_and_materials_features_from_preset(Domain::Preset::SelectedPreset& preset)
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
        auto& dest_mat = preset.hw_config.materials[Domain::Preset::Address{static_cast<uint8_t>(i)}];
        override_features(dest_mat.features, src_mat.features);
        std::visit(
            overloaded{
                [&dest_mat](const Domain::FilamentSettings& v) {
            auto it = v.contains("filament_type");
            if (it.item)
                dest_mat.type = it.item->get<std::string>();
        },
                [](const auto& v) {
        }
            },
            src_mat.values
        );
        dest_mat.id = src_mat.id;

        // TODO: fill values from MatDB here
        dest_mat.features["material_uuid"]  = "00000000-0000-0000-0000-000000000000";
        dest_mat.features["material_color"] = "#0070D0";
    }
}

void PresetInteractor::fill_config_container_with_selected_preset(
    Domain::ConfigContainer& cc,
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
)
{
    const auto* printer_preset = m_workbench.preset_bundle()
                                     .find_printer_preset(printer_hw_config_id, printer_preset_id);
    ASSERT(printer_preset != nullptr, printer_preset_id);
    const auto& print = printer_preset->prints.front();

    std::vector<Domain::Preset::EvaluatedToolPrintPreset::Preset> tools;
    for (const auto& t : print.tools) {
        // TODO: better choose tool-print preset + ask for config values transfer
        tools.emplace_back(t[0].preset);
    }

    std::vector<Domain::Preset::EvaluatedMaterialPreset::Preset> materials;
    for (const auto& m : print.materials) {
        // TODO: better choose tool-print preset + ask for config values transfer
        materials.emplace_back(m[0].preset);
    }

    auto& selected_preset = cc.mutable_selected_preset();
    selected_preset       = Domain::Preset::SelectedPreset{
              .hw_config = printer_preset->hw_config,
              .printer   = printer_preset->preset,
              .print     = print.preset,
              .tools     = tools,
              .materials = materials
    };
    update_hw_config_tools_and_materials_features_from_preset(selected_preset);
}

namespace {

template <typename T>
struct IsEvaluatedPreset : std::false_type
{};

template <typename T1, typename T2>
struct IsEvaluatedPreset<Domain::Preset::EvaluatedPreset<T1, T2>> : std::true_type
{};

template <typename T>
concept HavingEvaluatedPreset = requires(T t) {
    t.preset;
    requires IsEvaluatedPreset<decltype(t.preset)>::value;
};

template <HavingEvaluatedPreset T, std::output_iterator<PresetItem> Out>
void append_items(const std::vector<T>& source, const Domain::Preset::HwPrinterConfig& cfg, Out out)
{
    std::ranges::transform(source, out, [&](const T& preset) {
        return PresetItem{
            .id                     = preset.preset.id,
            .name                   = std::string{preset.preset.short_name()},
            .hw_printer_config_id   = cfg.id,
            .hw_printer_config_name = cfg.name,
            .runtime_only           = preset.preset.runtime_only
        };
    });
}

void set_items(PresetItemObservableList& dest, std::vector<PresetItem>&& items, size_t selected_index)
{
    ASSERT(selected_index != Domain::INVALID_ID);
    dest.items().set_items(std::move(items));
    dest.set_selected_index(selected_index);
}

std::optional<std::size_t> find_index_selected(
    const std::vector<PresetItem>& items,
    const std::string& item_id,
    const std::string& hw_config_id
)
{
    const auto it{std::ranges::find_if(items, [&](const PresetItem& item) {
        return item.hw_printer_config_id == hw_config_id && item.id == item_id;
    })};

    if (it == items.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(items.begin(), it));
}

template <HavingEvaluatedPreset T>
void set_items(
    PresetItemObservableList& dest,
    const std::vector<T>& source,
    const Domain::Preset::HwPrinterConfig& cfg,
    const std::string& selected_id
)
{
    std::vector<PresetItem> items;
    append_items<T>(source, cfg, std::back_inserter(items));

    if (items.empty()) {
        return;
    }
    const std::optional<std::size_t> selected_index{find_index_selected(items, selected_id, cfg.id)};

    if (!selected_index) {
        // TODO: now we cannot ensure the selected id is present, but once we have
        // more advanced selection state memory, we should be able to ensure this.

        // Just pick the first one.
        set_items(dest, std::move(items), 0);
        return;
    }

    set_items(dest, std::move(items), *selected_index);
}

} // namespace

void PresetInteractor::fill_printer_presets()
{
    const auto& preset_bundle = m_workbench.preset_bundle();

    std::vector<PresetItem> printers;
    for (const auto& [hw_config_id, ps] : preset_bundle.evaluated_presets) {
        if (ps.empty())
            continue;
        const auto& hw_config = ps.front().hw_config;
        append_items(ps, hw_config, std::back_inserter(printers));
    }

    const std::optional<std::size_t> selected_index{find_index_selected(
        printers,
        selected_printer_preset().printer.id,
        selected_printer_preset().hw_config.id
    )};

    ASSERT(selected_index, "Selected printer preset shuldd always be part of printers list!");
    set_items(m_printer_presets, std::move(printers), *selected_index);
}

void PresetInteractor::fill_print_presets(
    const Domain::Preset::EvaluatedPrinterPreset& printer_preset,
    Domain::Preset::SelectedPreset& selected_preset
)
{
    set_items(
        m_print_presets,
        printer_preset.prints,
        printer_preset.hw_config,
        selected_preset.print.id
    );
}

void PresetInteractor::fill_tools_presets(
    const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep,
    const Domain::Preset::EvaluatedPrintPreset& selected_print_ep,
    Domain::Preset::SelectedPreset& selected_preset
)
{
    std::vector<PresetItemObservableList> tools;

    const size_t tool_count = selected_print_ep.tools.size();
    ASSERT(
        selected_printer_ep.hw_config.technology != Domain::PrinterTechnology::FFF || tool_count > 0,
        selected_preset.print.id
    );
    ASSERT(tool_count == selected_preset.tools.size());
    for (auto [t, st] = std::tuple(selected_print_ep.tools.cbegin(), selected_preset.tools.cbegin());
         t != selected_print_ep.tools.cend();
         ++t, ++st)
    {
        PresetItemObservableList items;
        set_items(items, *t, selected_printer_ep.hw_config, st->id);
        tools.emplace_back(std::move(items));
    }
    m_tool_print_presets.set_items(std::move(tools));

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
        [](Domain::Preset::EvaluatedToolPrintPreset::Preset& preset) { return &preset.config_box(); }
    );

    SetAccessorMap accessors = m_tool_cbi_list.set_items(tool_cbs);
    m_cbi_accessors.insert(accessors.begin(), accessors.end());
}

void PresetInteractor::fill_materials_presets(
    const Domain::Preset::EvaluatedPrintPreset& selected_print_ep,
    Domain::Preset::SelectedPreset& selected_preset
)
{
    ASSERT(selected_print_ep.materials.size() == selected_preset.materials.size());
    std::vector<PresetItemObservableList> materials;
    for (auto [m, sm] =
             std::tuple(selected_print_ep.materials.begin(), selected_preset.materials.cbegin());
         m != selected_print_ep.materials.cend();
         ++m, ++sm)
    {
        PresetItemObservableList items;
        set_items(items, *m, selected_preset.hw_config, sm->id);
        materials.emplace_back(std::move(items));
    }
    m_material_presets.set_items(std::move(materials));

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

void PresetInteractor::fill_tool_items(const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep)
{
    HwConfigEvaluator config_eval;
    HwToolConfigIterator iterator = config_eval.iterate_tools(
        selected_printer_ep.hw_config,
        m_workbench.preset_bundle()
            .vendor_bundles.at(selected_printer_ep.hw_config.vendor_id)
            .vendor_data.defs.at(selected_printer_ep.technology())
            .tools
    );

    std::vector<Domain::Preset::HwToolConfigDef> tool_items;
    for (; iterator != std::end(iterator); ++iterator) {
        tool_items.push_back(*iterator);
    }
    std::vector<ToolConfigItemObservableList> items;
    items.reserve(selected_printer_ep.hw_config.tool_count);
    for (size_t tool_index = 0; tool_index < selected_printer_ep.hw_config.tool_count; ++tool_index)
    {
        ToolConfigItemObservableList& item = items.emplace_back();
        item.items().set_items(tool_items);

        // Find selected Tool
        const std::string selected_id = selected_printer_ep.hw_config.tools.at(tool_index).id;
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

void PresetInteractor::fill_sheet_items(const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep)
{
    HwConfigEvaluator config_eval;
    HwSheetConfigIterator iterator = config_eval.iterate_sheets(
        selected_printer_ep.hw_config,
        m_workbench.preset_bundle()
            .vendor_bundles.at(selected_printer_ep.hw_config.vendor_id)
            .vendor_data.defs.at(selected_printer_ep.technology())
            .sheets
    );

    size_t selected_index = 0;
    std::vector<Domain::Preset::HwSheetConfigDef> items;
    for (; iterator != std::end(iterator); ++iterator) {
        items.push_back(*iterator);
        if (iterator->id == selected_printer_ep.hw_config.sheet.id) {
            selected_index = std::distance(std::begin(iterator), iterator);
        }
    }
    m_sheet_items.items().set_items(items);
    if (!items.empty()) { // SLA doesn't yet have sheets specified
        m_sheet_items.set_selected_index(selected_index);
    }
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

    const Domain::Preset::EvaluatedPrinterPreset& p = current_printer_preset();
    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();
    fill_print_presets(p, selected_preset);

    m_printer_presets.set_selected([&printer_hw_config_id, &printer_preset_id](const PresetItem& item) {
        return item.id == printer_preset_id && item.hw_printer_config_id == printer_hw_config_id;
    });
    m_printer_cbi.set_config_box(&selected_preset.printer.config_box());
    m_print_cbi.set_config_box(&selected_preset.print.config_box());

    const auto* print = p.find_print_preset_by_id(selected_preset.print.id);
    ASSERT(print != nullptr, selected_preset.print.id);
    fill_tools_presets(p, *print, selected_preset);
    fill_materials_presets(*print, selected_preset);

    fill_tool_items(p);
    fill_sheet_items(p);

    // notify on change
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id, &cc](auto* l) {
        l->on_preset_selection_changed(project_id, cc->id().id, PresetItemType::PrinterPreset);
    });
    invoke_slicing_input_changed();
}

void PresetInteractor::select_print_preset(const std::string& id)
{
    Domain::Preset::SelectedPreset& selected_preset = mutable_selected_printer_presets();
    const auto* ep = m_workbench.preset_bundle().find_printer_preset(
        selected_preset.hw_config.id,
        selected_preset.printer.id
    );
    ASSERT(ep != nullptr, selected_preset.printer.id);

    const auto* p = ep->find_print_preset_by_id(id);
    ASSERT(p != nullptr, id);

    selected_preset.print = p->preset;

    const Domain::Preset::EvaluatedPrinterPreset& printer = current_printer_preset();
    selected_preset                                       = mutable_selected_printer_presets();

    m_print_presets.set_selected([&id](const PresetItem& item) { return item.id == id; });
    m_print_cbi.set_config_box(&selected_preset.print.config_box());

    const auto* print = printer.find_print_preset_by_id(selected_preset.print.id);
    ASSERT(print != nullptr, selected_preset.print.id);
    fill_tools_presets(*ep, *print, selected_preset);
    fill_materials_presets(*print, selected_preset);

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id,
                                              config_container_id](auto* l) {
        l->on_preset_selection_changed(project_id, config_container_id, PresetItemType::PrintPreset);
    });
    invoke_slicing_input_changed();
}

void PresetInteractor::select_tool_print_preset(size_t tool_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto* ep        = m_workbench.preset_bundle().find_printer_preset(
        selected_preset.hw_config.id,
        selected_preset.printer.id
    );
    ASSERT(ep != nullptr, selected_preset.printer.id);

    const auto* p = ep->find_print_preset_by_id(selected_preset.print.id);
    ASSERT(p != nullptr, id);

    ASSERT(tool_index < p->tools.size());
    ASSERT(tool_index < selected_preset.tools.size());
    const auto* t = p->find_tool_preset_by_id(tool_index, id);
    ASSERT(t, id);
    selected_preset.tools[tool_index] = t->preset;

    m_tool_print_presets_writer.mutate_at(tool_index, [&id](auto& item) {
        item.set_selected([&id](const PresetItem& item) { return item.id == id; });
    });

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id,
                                              config_container_id](auto* l) {
        l->on_preset_selection_changed(project_id, config_container_id, PresetItemType::ToolPrintPreset);
    });
    invoke_slicing_input_changed();
}

void PresetInteractor::select_material_preset(size_t material_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto* ep        = m_workbench.preset_bundle().find_printer_preset(
        selected_preset.hw_config.id,
        selected_preset.printer.id
    );
    ASSERT(ep != nullptr, selected_preset.printer.id);

    const auto* p = ep->find_print_preset_by_id(selected_preset.print.id);

    ASSERT(material_index < p->materials.size());
    ASSERT(material_index < selected_preset.materials.size());

    const auto* m = p->find_material_preset_by_id(material_index, id);
    ASSERT(m, id);

    selected_preset.materials[material_index] = m->preset;

    m_material_presets_writer.mutate_at(material_index, [&id](auto& item) {
        item.set_selected([&id](const PresetItem& item) { return item.id == id; });
    });

    const auto& ccc = selected_config_container_context();
    const Domain::SelectionId config_container_id{ccc.config_container_id};
    invoke_listeners<IPresetChangedListener>([project_id = m_selected_project_id,
                                              config_container_id](auto* l) {
        l->on_preset_selection_changed(project_id, config_container_id, PresetItemType::MaterialPreset);
    });
    invoke_slicing_input_changed();
}

void PresetInteractor::select_printer_tool_item(size_t tool_index, const std::string& id) {}

void PresetInteractor::select_printer_sheet(const std::string& id) {}

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
            config_box_opt = &std::get<Domain::ToolPrintSettings>(
                selected_preset.tools[element_idx].values
            );
            break;

        case Domain::FDMConfigLocation::Filament:
            config_box_opt = &std::get<Domain::FilamentSettings>(
                selected_preset.materials[element_idx].values
            );
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
    auto it          = config_box.contains(name);
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

void PresetInteractor::set_item_value(
    const Domain::ConfigItem& item,
    const Domain::ConfigValue& value,
    size_t index
)
{
    // This is a temporary dummy way how to set items, a whole dependency resolving with overrides needs
    // to be ported here from the Legacy code

    std::visit([=, this](auto&& location) {
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
            default:
                break;
            }
        }
    }, item.def().location);
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
        invoke_listeners<ISlicingInputChangedListener>([&](auto listener) {
            listener->on_slicing_input_changed({ccc.config_container_id, instance->id().id});
        });
    }
}

void PresetInteractor::invoke_on_preset_value_changed(const Domain::ConfigItem& config_item)
{
    const auto& ccc = selected_config_container_context();
    invoke_listeners<IPresetChangedListener>([&](auto listener) {
        listener->on_preset_value_changed(m_selected_project_id, ccc.config_container_id, config_item);
    });
}

PresetInteractorProjectContext& PresetInteractor::get_or_create_project_context(
    Domain::SelectionId project_id
)
{
    auto it = m_project_contexts.find(project_id);
    if (it != m_project_contexts.end())
        return it->second;

    bool _;
    std::tie(it, _) = m_project_contexts
                          .emplace(project_id, PresetInteractorProjectContext{project_id});

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
    std::tie(it, _) = project_context.config_containers.emplace(config_container_id, std::move(ccc));
    return it->second;
}

} // namespace Slic3r::Biz::Preset
