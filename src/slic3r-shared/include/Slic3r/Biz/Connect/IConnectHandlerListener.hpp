#pragma once

#include <string>

namespace Slic3r::Biz::Connect {

class IConnectHandlerListener 
{
public:
    virtual ~IConnectHandlerListener() = default;
    virtual void on_connect_requests_select_printer_preset(const std::string& printer_hw_config_id, const std::string& printer_preset_id) {}
    virtual void on_connect_requests_select_printer_tool_item(size_t tool_index, const std::string& id) {}
    virtual void on_connect_requests_select_material_preset(size_t material_index, const std::string& id) {}
    virtual void on_connect_handler_error(const std::string& msg) {}
};
} // namespace Slic3r::Biz::Connect