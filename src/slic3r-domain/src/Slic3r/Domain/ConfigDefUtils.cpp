#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigValue.hpp"

#include <algorithm>

namespace Slic3r::Domain {


std::vector<EnumValueDefsPtr>& get_enum_defs() {
    static std::vector<EnumValueDefsPtr> result;
    return result;
}

void append_enum_choice(
    ConfigDefinitions& defs,
    const std::string_view name,
    const int enum_value,
    std::string str_serialized,
    std::string str_ui
)
{
    ConfigItemDef* def = defs.find_mutable(name);
    ASSERT(def != nullptr);

    const ConfigValue seed{def->init_fn()};
    const EnumWrapper current{seed.get<EnumWrapper>()};

    EnumValueDefs choices{current.def()};
    choices.push_back({enum_value, std::move(str_serialized), std::move(str_ui)});
    std::sort(choices.begin(), choices.end());

    get_enum_defs().push_back(std::make_unique<EnumValueDefs>(std::move(choices)));

    def->init_fn = [value{current.value()}, type{current.type()},
                    stored{get_enum_defs().back().get()}]()
    { return ConfigValue{EnumWrapper{value, type, *stored}}; };
}

}
