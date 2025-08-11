#include "Slic3r/Biz/Preset/HwConfigEvaluator.hpp"
#include "Slic3r/Biz/Preset/ValueMapBuilder.hpp"
#include "Slic3r/Uuid.hpp"
#include "libslic3r/Config.hpp"

namespace Slic3r::Biz::Preset {

HwToolConfigIterator HwConfigEvaluator::iterate_tools(
    const Domain::Preset::HwPrinterConfig& printer,
    const HwToolConfigIterator::Container& tools
) const
{
    Expr::ValueMap values;
    append_printer_values(values, printer);
    return HwToolConfigIterator{tools, m_eval, std::move(values)};
}

HwFeederConfigIterator HwConfigEvaluator::iterate_feeders(
    const Domain::Preset::HwPrinterConfig& printer,
    const Domain::Preset::HwToolConfig& tool,
    const HwFeederConfigIterator::Container& feeders
) const
{
    Expr::ValueMap values;
    append_printer_values(values, printer);
    append_tool_values(values, tool);
    return HwFeederConfigIterator{feeders, m_eval, std::move(values)};
}

HwSheetConfigIterator HwConfigEvaluator::iterate_sheets(
    const Domain::Preset::HwPrinterConfig& printer,
    const HwSheetConfigIterator::Container& sheets
) const
{
    Expr::ValueMap values;
    append_printer_values(values, printer);
    return HwSheetConfigIterator{sheets, m_eval, std::move(values)};
}

Domain::Preset::HwPrinterConfig HwConfigEvaluator::create_printer_config(
    const Domain::Preset::HwPrinterConfigTemplate& templ,
    const Domain::Preset::VendorData& vendor_data
) const
{
    const auto* printer_def = vendor_data.find_printer_config_def_by_id(templ.printer);
    ASSERT(printer_def != nullptr, "Printer config not found", templ.printer);

    auto features = build_features(vendor_data.info.features.printer);
    Domain::Preset::override_features(features, printer_def->features);
    Domain::Preset::override_features(features, templ.features);

    Domain::Preset::VisualRepresentation visual = printer_def->visual;
    Domain::Preset::override_visual(visual, templ.visual);

    Domain::Preset::HwPrinterConfig printer_config{
        .id           = generate_uuid(),
        .printer_id   = printer_def->id,
        .vendor_id    = vendor_data.info.id,
        .repo_id      = vendor_data.info.repo_id,
        .repo_version = vendor_data.info.version,
        .name         = templ.name.empty() ? printer_def->name : templ.name,
        .technology   = printer_def->technology,
        .model        = printer_def->model,
        .tool_count   = templ.tool_count.has_value() ? templ.tool_count.value() :
                                                       printer_def->tool_count,
        .features     = features,
        .visual       = visual
    };

    ASSERT(templ.tools.size() == templ.tool_count || templ.tools.size() == 1, templ.id);
    for (const auto& tool_templ : templ.tools) {
        const auto* tool_def = vendor_data.find_tool_config_def_by_id(tool_templ.id);
        ASSERT(tool_def != nullptr, tool_templ.id);
        ASSERT(tool_def->technology == printer_def->technology, tool_templ.id);

        auto tool_features = build_features(vendor_data.info.features.tool);
        Domain::Preset::override_features(tool_features, tool_def->features);
        Domain::Preset::override_features(tool_features, tool_templ.features);
        Domain::Preset::HwToolConfig tool_config{
            .id       = tool_def->id,
            .features = tool_features,
        };
        printer_config.tools.emplace_back(std::move(tool_config));
    }

    // If only single tool provided, fill the rest slots with same tool-config
    while (printer_config.tools.size() < printer_config.tool_count)
        printer_config.tools.push_back(printer_config.tools.front());

    for (const auto& feeder_templ : templ.feeders) {
        const auto* feeder_def = vendor_data.find_feeder_config_def_by_id(feeder_templ.id);
        ASSERT(feeder_def != nullptr, feeder_templ.id);
        ASSERT(feeder_def->technology == printer_def->technology, feeder_templ.id);
        auto feeder_features = build_features(vendor_data.info.features.feeder);
        Domain::Preset::override_features(feeder_features, feeder_def->features);
        Domain::Preset::override_features(feeder_features, feeder_templ.features);
        Domain::Preset::HwFeederConfig feeder_config{
            .id         = feeder_def->id,
            .type       = feeder_def->type,
            .model      = feeder_def->model,
            .slot_count = feeder_def->slot_count,
            .features   = feeder_features,
        };
        printer_config.feeders.emplace(std::make_pair(feeder_templ.address, std::move(feeder_config)));
    }

    if (printer_config.technology == Domain::PrinterTechnology::FFF) {
        const auto* sheet_def = templ.sheet.has_value() ?
            vendor_data.find_sheet_config_def_by_id(*templ.sheet) :
            first_compatible_sheet(
                printer_config,
                vendor_data.defs.find(Domain::PrinterTechnology::FFF)->second.sheets
            );
        ASSERT(sheet_def != nullptr, sheet_def->id);
        auto& sheet    = printer_config.sheet;
        sheet.id       = sheet_def->id;
        sheet.name     = sheet_def->name;
        sheet.type     = sheet_def->type;
        sheet.features = Domain::Preset::build_features(vendor_data.info.features.sheet);
        Domain::Preset::override_features(sheet.features, sheet_def->features);
    }

    printer_config.name = Domain::Preset::suggest_name(printer_config, vendor_data);

    return printer_config;
}

const Domain::Preset::HwSheetConfigDef* HwConfigEvaluator::first_compatible_sheet(
    const Domain::Preset::HwPrinterConfig& printer,
    const HwSheetConfigIterator::Container& sheets
) const
{
    auto it = iterate_sheets(printer, sheets);
    return it == std::end(it) ? nullptr : &*it;
}

} // namespace Slic3r::Biz::Preset
