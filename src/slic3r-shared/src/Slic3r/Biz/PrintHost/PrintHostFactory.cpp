#include "Slic3r/Biz/PrintHost/PrintHostFactory.hpp"

#include "Slic3r/Biz/PrintHost/PrintHostLocal.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostOctoPrint.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostPrusaLink.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostPrusaLinkStorage.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostPrusaConnect.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostSL1Host.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostMoonraker.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostDuet.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostFlashAir.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostAstroBox.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostRepetier.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostMKS.hpp"


namespace Slic3r::Biz::PrintHost {

std::unique_ptr<IPrintHost> create_print_host(PrintHostConfig config, PrintHostJobData data)
{
    using PrintHostType = Domain::PrintHostType;

    switch (config.type) {
        case PrintHostType::Local: return std::make_unique<PrintHostLocal>(std::move(config), std::move(data));
        case PrintHostType::OctoPrint: return std::make_unique<PrintHostOctoPrint>(std::move(config), std::move(data));
        case PrintHostType::PrusaLink: return std::make_unique<PrintHostPrusaLink>(std::move(config), std::move(data));
        case PrintHostType::PrusaLinkStorage: return std::make_unique<PrintHostPrusaLinkStorage>(std::move(config), std::move(data));
        case PrintHostType::SL1Host: return std::make_unique<PrintHostSL1Host>(std::move(config), std::move(data));
        case PrintHostType::Moonraker: return std::make_unique<PrintHostMoonraker>(std::move(config), std::move(data));
        case PrintHostType::Duet: return std::make_unique<PrintHostDuet>(std::move(config), std::move(data));
        case PrintHostType::FlashAir: return std::make_unique<PrintHostFlashAir>(std::move(config), std::move(data));
        case PrintHostType::AstroBox: return std::make_unique<PrintHostAstroBox>(std::move(config), std::move(data));
        case PrintHostType::Repetier: return std::make_unique<PrintHostRepetier>(std::move(config), std::move(data));
        case PrintHostType::MKS: return std::make_unique<PrintHostMKS>(std::move(config), std::move(data));
        case PrintHostType::PrusaConnect: return std::make_unique<PrintHostPrusaConnect>(std::move(config), std::move(data));
        default: ASSERT(false); return nullptr;
     }
    return nullptr;
}

} // Slic3r::Biz::PrintHost 