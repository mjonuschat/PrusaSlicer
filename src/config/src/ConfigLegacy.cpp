#include "libslicerconfig/ConfigLegacy.hpp"
#include "libslicerconfig/Config.hpp"

#include "Legacy/PrintConfig.hpp"

static void convert_enum(const Slic3r::ConfigOption* co, ConfigItem& item)
{
	const std::string old_str = co->serialize();
	for (const EnumValueDef& evd : item.def().enum_values)
		if (evd.str_serialized == old_str)
			item.set_enum_from_string(old_str);
}





void load_from_legacy_file(const std::string& filename, ConfigBox& box)
{
	Slic3r::DynamicPrintConfig cfg;
	Slic3r::ForwardCompatibilitySubstitutionRule substitutions_ctxt = Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent;
	cfg.load(filename, substitutions_ctxt);
	// TODO handle errors

	for (const auto& key : cfg.keys()) {
		if (! box.has(key))
			continue;
		Slic3r::ConfigOption* opt = cfg.option(key);
		ConfigItem& item = box.opt(key);
		
		using namespace Slic3r;
		if (opt->nullable()) {
			bool old_nil = opt->is_nil();
			bool nullable_new = std::find(item.def().belongs_to_optional.begin(), item.def().belongs_to_optional.end(), box.type()) != item.def().belongs_to_optional.end();

			if (nullable_new) {
				item.set_null(old_nil);
				if (old_nil)
					return;
			}
		}

		if (opt->type() == coBool && item.type() == ConfigItemType::Bool)
			item.set_bool(opt->getBool());
		else if (opt->type() == coInt && item.type() == ConfigItemType::Int)
			item.set_int(opt->getInt());
		else if (opt->type() == coFloat && item.type() == ConfigItemType::Double)
			item.set_double(opt->getFloat());
		else if (opt->type() == coString && item.type() == ConfigItemType::String)
			item.set_str(static_cast<ConfigOptionString*>(opt)->value);
		else if (opt->type() == coEnum && item.type() == ConfigItemType::Enum)
			convert_enum(opt, item);
		else {
			// Old and new types do not match.
		}
	}

}