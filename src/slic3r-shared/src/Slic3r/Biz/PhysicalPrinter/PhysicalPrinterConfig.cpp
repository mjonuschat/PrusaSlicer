#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterConfig.hpp"

#include "Slic3r/Domain/ConfigPhysical.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Slic3r::Biz::PhysicalPrinter {


std::string physical_printer_type_to_string(const PhysicalPrinterConfig& data)
{
    if (const auto* local = std::get_if<LocalAuth>(&data.connection_data); local) {
        return print_host_type_to_string(local->type);
    } else if (const auto* cloud = std::get_if<CloudAuth>(&data.connection_data); cloud) {
        return "Connect";
    } else {
        return {};
    }
}

PhysicalPrinterConfig none()
{
    return {OperationType::None, _u8L("No Physical Printer"), boost::uuids::to_string(boost::uuids::random_generator()())};
}

} //namespace Slic3r::Biz::PhysicalPrinter