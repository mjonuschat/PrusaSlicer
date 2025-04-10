#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Domain/Config.hpp"

#include "Legacy/PrintConfig.hpp"


static void convert_enum(const Slic3rLegacy::ConfigOption* co, ConfigItem& item)
{
    const std::string old_str = co->serialize();
    for (const EnumValueDef& evd : item.def().enum_values)
        if (evd.str_serialized == old_str)
            item.set_enum_from_string(old_str);
}





void load_from_legacy_file(const std::string& filename, ConfigBox& box)
{
    using namespace Slic3rLegacy;
    DynamicPrintConfig cfg;
    ForwardCompatibilitySubstitutionRule substitutions_ctxt = ForwardCompatibilitySubstitutionRule::EnableSilent;
    cfg.load(filename, substitutions_ctxt);
    // TODO handle errors

    for (const auto& key : cfg.keys()) {
        if (! box.has(key))
            continue;
        ConfigOption* opt = cfg.option(key);
        ConfigItem& item = box.opt(key);
        
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

        if (opt->type() == coBool && item.type() == ConfigItemType::Bool)
            item.set<bool>(opt->getBool());
        else if (opt->type() == coInt && item.type() == ConfigItemType::Int)
            item.set<int>(opt->getInt());
        else if (opt->type() == coFloat && item.type() == ConfigItemType::Double)
            item.set<double>(opt->getFloat());
        else if (opt->type() == coString && item.type() == ConfigItemType::String)
            item.set<std::string>(static_cast<ConfigOptionString*>(opt)->value);
        else if (opt->type() == coEnum && item.type() == ConfigItemType::Enum)
            convert_enum(opt, item);
        else {
            // Old and new types do not match.
        }
    }

}
