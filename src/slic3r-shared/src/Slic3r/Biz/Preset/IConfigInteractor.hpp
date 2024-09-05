#pragma once

#include <functional>
#include <boost/any.hpp>

namespace Slic3r {
class DynamicPrintConfig;
}

namespace Slic3r::Biz::Preset {

class IConfigInteractor
{
public:
    virtual ~IConfigInteractor() = default;

    virtual const DynamicPrintConfig& config() const = 0;
    virtual void set_config_value(
        const std::string& name, const boost::any& value, int opt_index
    ) = 0;
    virtual void set_config_num_extruders(size_t num_extruders) = 0;
    virtual void set_config(const DynamicPrintConfig& config) = 0;

    using ModifyFunc = std::function<void(DynamicPrintConfig&)>;
    virtual void modify_config(ModifyFunc mod_fn) = 0;
};

}
