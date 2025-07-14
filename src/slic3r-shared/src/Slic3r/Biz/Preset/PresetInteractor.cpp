#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Preset/IBedPresetValueChangedListener.hpp"
#include "Slic3r/Biz/Preset/IBedPresetSwitchedListener.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/App/Platform/KeyCode.hpp"
#include "Slic3r/Biz/Preset/HwConfigEvaluator.hpp"
#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/IO/BundleLoader.hpp"
#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"

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
        }
    }
    size_t tool_idx = 0;
    SPDLOG_INFO("-------------------------------------------------");
    SPDLOG_INFO("Materials:");
    SPDLOG_INFO("-------------------------------------------------");
    for (const auto& tool_mats : preset.materials) {
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

} // namespace

void PresetInteractor::load_preset_bundle(const std::string& preset_bundle_path, const std::string& config_path)
{
    auto preset_bundle = IO::load_bundle(preset_bundle_path, config_path);

    // TODO: remove this when config wizard is ready
    if (preset_bundle.printer_configs.empty()) {
        HwConfigEvaluator config_eval;

        auto& prusa_fff = preset_bundle.vendor_bundles["prusa-fff"];
        for (const auto& hw_printer_template : prusa_fff.vendor_data.printer_configs) {
            auto printer_config = config_eval.create_printer_config(
                hw_printer_template,
                prusa_fff.vendor_data
            );
            preset_bundle.printer_configs.emplace(printer_config.id, printer_config);
            prusa_fff.printer_configs.emplace_back(std::move(printer_config));
        }
    }

    preset_bundle.evaluated_presets.clear();
    for (const auto& [vendor_id, vendor_bundle] : preset_bundle.vendor_bundles) {
        PresetEvaluator preset_evaluator{vendor_bundle.presets};
        for (const auto& p : vendor_bundle.printer_configs) {
            auto epps = preset_evaluator.evaluate(p);
            for (auto& epp : epps) {
                dump_ep_info(epp);
                preset_bundle.evaluated_presets[epp.hw_config.id].emplace_back(std::move(epp));
            }
        }
    }
    // do not save it now, as we create it anyway again
    // IO::save_bundle_configs(preset_bundle, config_path);

    m_workbench.set_preset_bundle(std::move(preset_bundle));
    if (m_selected_project_id != Domain::INVALID_ID)
        fill_printer_presets();
}

void PresetInteractor::prepare_config_container_preset(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    auto& ccc = get_or_create_config_container_context(project_id, config_container_id);
}

void PresetInteractor::initialize_config_container(Domain::ConfigContainer& cc)
{
    const auto& preset_bundle     = m_workbench.preset_bundle();
    const auto& evaluated_presets = preset_bundle.evaluated_presets;
    auto config_it                = preset_bundle.printer_configs.begin();
    while (config_it != preset_bundle.printer_configs.end()) {
        if (evaluated_presets.contains(config_it->second.id))
            break;
        ++config_it;
    }
    ASSERT(config_it != preset_bundle.printer_configs.end());

    const auto& printer_presets = evaluated_presets.at(config_it->second.id);
    const auto printer_it       = printer_presets.begin();
    ASSERT(printer_it != printer_presets.end());

    fill_config_container_with_selected_preset(cc, printer_it->preset.id);
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

    // TODO: remove this legacy
    ccc.preset_bundle_runtime
        .update_compatible_prints(m_workbench.preset_bundle_legacy(), *ccc.printer.selected_preset);
    ccc.preset_bundle_runtime.update_compatible_materials(
        m_workbench.preset_bundle_legacy(),
        *ccc.printer.selected_preset,
        *ccc.print.selected_preset
    );

    fill_printer_presets();
    const auto& p = current_printer_preset();
    const auto& s = selected_printer_preset();

    const auto* print = p.find_print_preset_by_id(s.print.id);
    ASSERT(print != nullptr, s.print.id);

    fill_print_presets(p, s);
    fill_tools_presets(p, *print, s);
    fill_materials_presets(p, s);

    // notify listeners on changes
    m_bed_preset_value_changed_listeners.invoke([&ccc](auto* l) {
        l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_PRINT, ccc.print);
        l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_PRINTER, ccc.printer);
        if (!ccc.materials.empty())
            l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_FILAMENT, ccc.materials[0]);
    });
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

Domain::Preset::SelectedPreset& PresetInteractor::mutable_selected_printer_presets()
{
    auto& project   = m_workbench.project(m_selected_project_id);
    const auto& ccc = selected_config_container_context();
    auto* cc        = project.find_config_container(ccc.config_container_id);
    return cc->mutable_selected_preset();
}

void PresetInteractor::fill_config_container_with_selected_preset(
    Domain::ConfigContainer& cc,
    const std::string& printer_preset_id
)
{
    const auto* printer_preset = m_workbench.preset_bundle().find_printer_preset_by_id(
        printer_preset_id
    );
    ASSERT(printer_preset != nullptr, printer_preset_id);
    const auto& print = printer_preset->prints.front();

    std::vector<Domain::Preset::EvaluatedToolPrintPreset::Preset> tools;
    for (const auto& t : print.tools) {
        // TODO: better choose tool-print preset + ask for config values transfer
        tools.emplace_back(t[0].preset);
    }

    std::vector<Domain::Preset::EvaluatedMaterialPreset::Preset> materials;
    for (const auto& m : printer_preset->materials) {
        // TODO: better choose tool-print preset + ask for config values transfer
        materials.emplace_back(m[0].preset);
    }

    cc.mutable_selected_preset() = Domain::Preset::SelectedPreset{
        .hw_config = printer_preset->hw_config,
        .printer   = printer_preset->preset,
        .print     = print.preset,
        .tools     = tools,
        .materials = materials
    };
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

template <HavingEvaluatedPreset T>
void append_items(
    std::vector<PresetItem>& dest,
    const std::vector<T>& source,
    const Domain::Preset::HwPrinterConfig& cfg,
    const std::string& selected_id,
    size_t& idx,
    size_t& selected_index
)
{
    selected_index = size_t(-1);
    for (const auto& p : source) {
        dest.emplace_back(
            PresetItem{
                .id                     = p.preset.id,
                .name                   = p.preset.name,
                .hw_printer_config_id   = cfg.id,
                .hw_pritner_config_name = cfg.name
            }
        );
        if (p.preset.id == selected_id)
            selected_index = idx;
        idx++;
    }
}

void set_items(PresetItemObservableList& dest, std::vector<PresetItem> items, size_t selected_index)
{
    dest.items().set_items(std::move(items));
    dest.set_selected_index(selected_index);
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
    size_t idx = 0;
    size_t selected_index;
    append_items<T>(items, source, cfg, selected_id, idx, selected_index);
    set_items(dest, items, selected_index);
}

} // namespace

void PresetInteractor::fill_printer_presets()
{
    const auto& printer_preset_id = selected_printer_preset().printer.id;
    std::vector<PresetItem> printers;
    const auto& preset_bundle = m_workbench.preset_bundle();
    size_t idx                = 0;
    size_t selected_index     = Domain::INVALID_ID;
    for (const auto& [hw_config_id, ps] : preset_bundle.evaluated_presets) {
        if (ps.empty())
            continue;
        const auto& hw_config = ps.front().hw_config;
        append_items(printers, ps, hw_config, printer_preset_id, idx, selected_index);
    }
    set_items(m_printer_presets, printers, selected_index);
}

void PresetInteractor::fill_print_presets(
    const Domain::Preset::EvaluatedPrinterPreset& p,
    const Domain::Preset::SelectedPreset& s
)
{
    set_items(m_print_presets, p.prints, p.hw_config, s.print.id);
}

void PresetInteractor::fill_tools_presets(
    const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep,
    const Domain::Preset::EvaluatedPrintPreset& selected_print_ep,
    const Domain::Preset::SelectedPreset& s
)
{
    std::vector<PresetItemObservableList> tools;

    const size_t tool_count = selected_print_ep.tools.size();
    ASSERT(tool_count > 0, s.print.id);
    ASSERT(tool_count == s.tools.size());
    for (auto [t, st] = std::tuple(selected_print_ep.tools.cbegin(), s.tools.cbegin());
         t != selected_print_ep.tools.cend();
         ++t, ++st)
    {
        PresetItemObservableList items;
        set_items(items, *t, selected_printer_ep.hw_config, st->id);
        tools.emplace_back(std::move(items));
    }
    m_tool_print_presets.set_items(std::move(tools));
}

void PresetInteractor::fill_materials_presets(
    const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep,
    const Domain::Preset::SelectedPreset& s
)
{
    ASSERT(selected_printer_ep.materials.size() == s.materials.size());
    std::vector<PresetItemObservableList> materials;
    for (auto [m, sm] = std::tuple(selected_printer_ep.materials.begin(), s.materials.cbegin());
         m != selected_printer_ep.materials.cend();
         ++m, ++sm)
    {
        PresetItemObservableList items;
        set_items(items, *m, selected_printer_ep.hw_config, sm->id);
        materials.emplace_back(std::move(items));
    }
    m_material_presets.set_items(std::move(materials));
}

void PresetInteractor::select_printer_preset(const std::string& printer_preset_id)
{
    auto& project   = m_workbench.project(m_selected_project_id);
    const auto& ccc = selected_config_container_context();
    auto* cc        = project.find_config_container(ccc.config_container_id);
    ASSERT(cc != nullptr, ccc.config_container_id);

    fill_config_container_with_selected_preset(*cc, printer_preset_id);

    // notify on change
    const auto& p = current_printer_preset();
    const auto& s = selected_printer_preset();
    fill_print_presets(p, s);

    m_printer_presets.set_selected([&printer_preset_id](const PresetItem& item) {
        return item.id == printer_preset_id;
    });

    const auto* print = p.find_print_preset_by_id(s.print.id);
    ASSERT(print != nullptr, s.print.id);
    fill_tools_presets(p, *print, s);
    fill_materials_presets(p, s);
}

void PresetInteractor::select_print_preset(const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto* ep = m_workbench.preset_bundle().find_printer_preset_by_id(selected_preset.printer.id);
    ASSERT(ep != nullptr, selected_preset.printer.id);

    const auto* p = ep->find_print_preset_by_id(id);
    ASSERT(p != nullptr, id);

    selected_preset.print = p->preset;

    // notify on change
    const auto& printer = current_printer_preset();
    const auto& s       = selected_printer_preset();

    m_print_presets.set_selected([&id](const PresetItem& item) { return item.id == id; });

    const auto* print = printer.find_print_preset_by_id(s.print.id);
    ASSERT(print != nullptr, s.print.id);
    fill_tools_presets(*ep, *print, s);
    fill_materials_presets(*ep, s);
}

void PresetInteractor::select_tool_print_preset(size_t tool_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto* ep = m_workbench.preset_bundle().find_printer_preset_by_id(selected_preset.printer.id);
    ASSERT(ep != nullptr, selected_preset.printer.id);

    const auto* p = ep->find_print_preset_by_id(id);
    ASSERT(p != nullptr, id);

    ASSERT(tool_index < p->tools.size());
    ASSERT(tool_index < selected_preset.tools.size());
    const auto* t = p->find_tool_preset_by_id(tool_index, id);
    ASSERT(t, id);
    selected_preset.tools[tool_index] = t->preset;

    // notify on change
    m_tool_print_presets_writer.mutate_at(tool_index, [&id](auto& item) {
        item.set_selected([&id](const PresetItem& item) { return item.id == id; });
    });
}

void PresetInteractor::select_material_preset(size_t material_index, const std::string& id)
{
    auto& selected_preset = mutable_selected_printer_presets();
    const auto* ep = m_workbench.preset_bundle().find_printer_preset_by_id(selected_preset.printer.id);
    ASSERT(ep != nullptr, selected_preset.printer.id);

    ASSERT(material_index < ep->materials.size());
    ASSERT(material_index < selected_preset.materials.size());

    const auto* m = ep->find_material_preset_by_id(material_index, id);
    ASSERT(m, id);

    selected_preset.materials[material_index] = m->preset;

    // notify on change
    m_material_presets_writer.mutate_at(material_index, [&id](auto& item) {
        item.set_selected([&id](const PresetItem& item) { return item.id == id; });
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

    // TODO: notify on change
}

void PresetInteractor::set_legacy_preset_state_value(
    Slic3r::Preset::Type preset_type,
    size_t preset_index,
    const std::string& name,
    const boost::any& value,
    int opt_index
)
{
    auto& ccc          = mutable_selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config       = preset_state.edited_preset.config;

    set_config_value(config, name, value, opt_index);

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state);
    });

    const auto& project = m_workbench.project(m_selected_project_id);
    for (const auto& instance :
         project.find_config_container(ccc.config_container_id)->bed_instances())
    {
        m_slicing_input_changed_listeners.invoke([&](auto listener) {
            listener->on_slicing_input_changed({ccc.config_container_id, instance->id().id});
        });
    }
}

void PresetInteractor::set_legacy_preset_state_config_num_extruders(
    Slic3r::Preset::Type preset_type,
    size_t preset_index,
    size_t num_extruders
)
{
    auto& ccc          = mutable_selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config       = preset_state.edited_preset.config;

    config.set_num_extruders(num_extruders);

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state);
    });
}

void PresetInteractor::set_legacy_preset_state(
    Slic3r::Preset::Type preset_type,
    size_t preset_index,
    const DynamicPrintConfig& config
)
{
    auto& ccc          = mutable_selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);

    if (preset_state.edited_preset.config.diff(config).empty())
        return;

    preset_state.edited_preset.config = config;

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state);
    });
}

void PresetInteractor::modify_legacy_preset_state(
    Slic3r::Preset::Type preset_type,
    size_t preset_index,
    IConfigInteractor::ModifyFunc modify_fn
)
{
    auto& ccc          = mutable_selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config       = preset_state.edited_preset.config;
    auto orig_config   = config;

    modify_fn(config);

    if (!orig_config.diff(config).empty()) {
        m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
            l->on_bed_preset_value_changed(preset_type, preset_state);
        });
    }
}

void PresetInteractor::select_legacy_printer_preset(size_t preset_idx)
{
    auto& ccc                   = mutable_selected_config_container_context();
    PresetBundle& preset_bundle = m_workbench.preset_bundle_legacy();
    preset_bundle.printers.select_preset(preset_idx);
    ccc.printer = create_preset_state(preset_bundle.printers);

    // update PresetBundleRuntime
    ccc.preset_bundle_runtime.update_compatible_prints(preset_bundle, *ccc.printer.selected_preset);

    m_bed_preset_switched_listeners.invoke([](auto* l) {
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_PRINTER);
    });

    // TODO: select relevant print if needed
}

void PresetInteractor::select_legacy_print_preset(size_t preset_idx)
{
    auto& ccc                    = mutable_selected_config_container_context();
    PresetBundle& preset_bundle  = m_workbench.preset_bundle_legacy();
    bool is_sla                  = ccc.printer_technology() == ptSLA;
    PresetCollection& collection = is_sla ? preset_bundle.sla_prints : preset_bundle.prints;
    collection.select_preset(preset_idx);
    ccc.print = create_preset_state(collection);

    // TODO: update PresetBundleRuntime

    m_bed_preset_switched_listeners.invoke([](auto* l) {
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_PRINT);
    });
}

void PresetInteractor::select_legacy_extruder_preset(size_t extruder_idx, size_t preset_idx)
{
    // TODO: implement once extruder preset is available
}

void PresetInteractor::select_legacy_material_preset(size_t extruder_idx, size_t preset_idx)
{
    // TODO: update PresetBundleRuntime
    auto& ccc                    = mutable_selected_config_container_context();
    PresetBundle& preset_bundle  = m_workbench.preset_bundle_legacy();
    bool is_sla                  = ccc.printer_technology() == ptSLA;
    PresetCollection& collection = is_sla ? preset_bundle.sla_materials : preset_bundle.filaments;
    collection.select_preset(preset_idx);
    ccc.materials[extruder_idx] = create_preset_state(collection);

    m_bed_preset_switched_listeners.invoke([](auto* l) {
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_FILAMENT);
    });
}

void PresetInteractor::set_config_value(
    DynamicPrintConfig& config,
    const std::string& name,
    const boost::any& value,
    int opt_index
)
{
    if (config.def()->get(name)->type == coBools && config.def()->get(name)->nullable) {
        ConfigOptionBoolsNullable* vec_new = new ConfigOptionBoolsNullable{
            boost::any_cast<unsigned char>(value)
        };
        config.option<ConfigOptionBoolsNullable>(name)->set_at(vec_new, opt_index, 0);
        return;
    }

    const ConfigOptionDef* opt_def = config.def()->get(name);
    switch (opt_def->type) {
    case coFloatOrPercent: {
        std::string str = boost::any_cast<std::string>(value);
        bool percent    = false;
        if (str.back() == '%') {
            str.pop_back();
            percent = true;
        }
        double val = std::stod(
            str
        ); // locale-dependent (on purpose - the input is the actual content of the field)
        config.set_key_value(name, new ConfigOptionFloatOrPercent(val, percent));
        break;
    }
    case coPercent:
        config.set_key_value(name, new ConfigOptionPercent(boost::any_cast<double>(value)));
        break;
    case coFloat: {
        double& val = config.opt_float(name);
        val         = boost::any_cast<double>(value);
        break;
    }
    case coFloatsOrPercents: {
        std::string str = boost::any_cast<std::string>(value);
        bool percent    = false;
        if (str.back() == '%') {
            str.pop_back();
            percent = true;
        }

        const bool is_na_value   = opt_def->nullable && str == "N/A";
        const FloatOrPercent val = is_na_value ? ConfigOptionFloatsOrPercentsNullable::nil_value() :
                                                 FloatOrPercent{std::stod(str), percent};
        ConfigOptionFloatsOrPercents* vec_new = new ConfigOptionFloatsOrPercents({val});
        config.option<ConfigOptionFloatsOrPercents>(name)->set_at(vec_new, opt_index, opt_index);
        break;
    }
    case coPercents: {
        ConfigOptionPercents* vec_new = new ConfigOptionPercents{boost::any_cast<double>(value)};
        config.option<ConfigOptionPercents>(name)->set_at(vec_new, opt_index, opt_index);
        break;
    }
    case coFloats: {
        ConfigOptionFloats* vec_new = new ConfigOptionFloats{boost::any_cast<double>(value)};
        config.option<ConfigOptionFloats>(name)->set_at(vec_new, opt_index, opt_index);
        break;
    }
    case coString:
        config.set_key_value(name, new ConfigOptionString(boost::any_cast<std::string>(value)));
        break;
    case coStrings: {
        if (name == "compatible_prints"
            || name == "compatible_printers"
            || name == "gcode_substitutions")
        {
            config.option<ConfigOptionStrings>(name)
                ->values = boost::any_cast<std::/*__1::*/ vector<std::string>>(value);
        } else if (config.def()->get(name)->gui_flags.compare("serialized") == 0) {
            std::string str = boost::any_cast<std::string>(value);
            std::/*__1::*/ vector<std::string> values{};
            if (!str.empty()) {
                if (str.back() == ';')
                    str.pop_back();
                // Split a string to multiple strings by a semi - colon.This is the old way of storing multi - string values.
                // Currently used for the post_process config value only.
                boost::split(values, str, boost::is_any_of(";"));
                if (values.size() == 1 && values[0] == "")
                    values.resize(0);
            }
            config.option<ConfigOptionStrings>(name)->values = values;
        } else {
            ConfigOptionStrings* vec_new = new ConfigOptionStrings{boost::any_cast<std::string>(value)};
            config.option<ConfigOptionStrings>(name)->set_at(vec_new, opt_index, 0);
        }
    } break;
    case coBool:
        config.set_key_value(name, new ConfigOptionBool(boost::any_cast<bool>(value)));
        break;
    case coBools: {
        ConfigOptionBools* vec_new = new ConfigOptionBools{boost::any_cast<unsigned char>(value) != 0};
        config.option<ConfigOptionBools>(name)->set_at(vec_new, opt_index, 0);
        break;
    }
    case coInt: {
        // config.set_key_value(name, new ConfigOptionInt(boost::any_cast<int>(value)));
        int& val_new = config.opt_int(name);
        val_new      = boost::any_cast<int>(value);
        break;
    }
    case coInts: {
        ConfigOptionInts* vec_new = new ConfigOptionInts{boost::any_cast<int>(value)};
        config.option<ConfigOptionInts>(name)->set_at(vec_new, opt_index, 0);
    } break;
    case coEnum: {
        auto* opt = opt_def->default_value.get()->clone();
        opt->setInt(boost::any_cast<int>(value));
        config.set_key_value(name, opt);
    } break;
    case coEnums: {
        ConfigOptionEnumsGeneric* vec_new = new ConfigOptionEnumsGeneric(1, boost::any_cast<int>(value));
        config.option<ConfigOptionEnumsGeneric>(name)->set_at(vec_new, opt_index, 0);
        break;
    }
    case coPoints: {
        if (name == "bed_shape") {
            config.option<ConfigOptionPoints>(name)->values = boost::any_cast<std::vector<Vec2d>>(
                value
            );
            break;
        }
        ConfigOptionPoints* vec_new = new ConfigOptionPoints{boost::any_cast<Vec2d>(value)};
        config.option<ConfigOptionPoints>(name)->set_at(vec_new, opt_index, 0);
    } break;
    case coNone:
        break;
    default:
        break;
    }
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

    // fill ccc presets from cc.print_config
    PresetBundle& preset_bundle = m_workbench.preset_bundle_legacy();
    auto pt                     = preset_bundle.printers.get_selected_preset().printer_technology();
    preset_bundle.load_config_model(project.file_name(), cc.print_config());
    ccc.printer = create_preset_state(preset_bundle.printers);
    ccc.print   = create_preset_state(
        pt == PrinterTechnology::ptFFF ? preset_bundle.prints : preset_bundle.sla_prints
    );
    ccc.materials = {create_preset_state(preset_bundle.materials(pt))};
    ccc.preset_bundle_runtime.update_compatible_prints(preset_bundle, ccc.printer.edited_preset);
    ccc.preset_bundle_runtime.update_compatible_materials(
        preset_bundle,
        ccc.printer.edited_preset,
        ccc.print.edited_preset
    );

    bool _;
    std::tie(it, _) = project_context.config_containers.emplace(config_container_id, std::move(ccc));
    return it->second;
}

PresetState PresetInteractor::create_preset_state(Slic3r::Preset* selected_preset)
{
    auto& collection = m_workbench.preset_bundle_legacy().get_presets(selected_preset->type);
    return {selected_preset, collection.get_preset_parent(*selected_preset)};
}

PresetState PresetInteractor::create_preset_state(PresetCollection& source_with_selected)
{
    auto& preset = source_with_selected.get_selected_preset();
    return {&preset, source_with_selected.get_selected_preset_parent()};
}

const DynamicPrintConfig& LegacyPresetConfigInteractor::config() const
{
    return m_parent.selected_config_container_context()
        .preset_state(m_preset_type, m_preset_index)
        .edited_preset.config;
}

const PresetState& LegacyPresetConfigInteractor::legacy_preset_state() const
{
    return m_parent.selected_config_container_context().preset_state(m_preset_type, m_preset_index);
}

void LegacyPresetConfigInteractor::set_config_value(
    const std::string& name,
    const boost::any& value,
    int opt_index
)
{
    m_parent.set_legacy_preset_state_value(m_preset_type, m_preset_index, name, value, opt_index);
}

void LegacyPresetConfigInteractor::set_config_num_extruders(size_t num_extruders)
{
    m_parent.set_legacy_preset_state_config_num_extruders(m_preset_type, m_preset_index, num_extruders);
}

void LegacyPresetConfigInteractor::set_config(const Slic3r::DynamicPrintConfig& config)
{
    m_parent.set_legacy_preset_state(m_preset_type, m_preset_index, config);
}

void LegacyPresetConfigInteractor::modify_config(IConfigInteractor::ModifyFunc mod_fn)
{
    m_parent.modify_legacy_preset_state(m_preset_type, m_preset_index, mod_fn);
}

} // namespace Slic3r::Biz::Preset
