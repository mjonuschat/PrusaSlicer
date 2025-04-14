#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Domain/Config.hpp"

#include "Legacy/PrintConfig.hpp"

namespace Slic3r::Biz {

static void convert_enum(const Slic3rLegacy::ConfigOption* co, Domain::ConfigItem& item)
{
    const std::string old_str = co->serialize();
    for (const Domain::EnumValueDef& evd : item.def().enum_values)
        if (evd.str_serialized == old_str)
            item.set_enum_from_string(old_str);
}





void load_from_legacy_file(const std::string& filename, Domain::ConfigBox& box)
{
    PANIC("This function is not finished. Don't use it yet.");

    using namespace Slic3rLegacy;
    DynamicPrintConfig cfg;
    ForwardCompatibilitySubstitutionRule substitutions_ctxt = ForwardCompatibilitySubstitutionRule::EnableSilent;
    cfg.load(filename, substitutions_ctxt);
    // TODO handle errors

    for (const auto& key : cfg.keys()) {
        if (! box.has(key))
            continue;
        ConfigOption* opt = cfg.option(key);
        Domain::ConfigItem& item = box.opt(key);
        
        if (opt->nullable()) {
            bool old_nil = opt->is_nil();
            bool nullable_new = std::find(item.def().belongs_to_optional.begin(), item.def().belongs_to_optional.end(), box.type()) != item.def().belongs_to_optional.end();
            // TODO: We really should enumerate all nullables present in old slicer and handle them separately.
            // Otherwise, the new slicer will not know in which box shall the nullable item be set.

            if (nullable_new) {
                item.set_null(old_nil);
                if (old_nil)
                    return;
            }
        }

        if (opt->type() == coBool && item.type() == Domain::ConfigItemType::Bool)
            item.set<bool>(opt->getBool());
        else if (opt->type() == coInt && item.type() == Domain::ConfigItemType::Int)
            item.set<int>(opt->getInt());
        else if (opt->type() == coFloat && item.type() == Domain::ConfigItemType::Double)
            item.set<double>(opt->getFloat());
        else if (opt->type() == coString && item.type() == Domain::ConfigItemType::String)
            item.set<std::string>(static_cast<ConfigOptionString*>(opt)->value);
        else if (opt->type() == coEnum && item.type() == Domain::ConfigItemType::Enum)
            convert_enum(opt, item);
        else {
            // Old and new types do not match.
        }
    }
}


// TODO: New slicer changed enums PrintHostType and AuthorizationType (=PrintHostAuthType).
// We need to convert old options to the new ones properly. BEWARE especially of PrusaConnect and
// PrusaConnectNew. PrusaConnect was removed and PrusaConnectNew was renamed to PrusaConnect.

} // namespace Slic3r::Biz
