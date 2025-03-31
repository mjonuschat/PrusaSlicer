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

std::unique_ptr<IPrintHost> create_print_host(PrintHostConfig config)
{
    switch (config.type) {
        case PrintHostType::Local: return std::make_unique<PrintHostLocal>(std::move(config));
        case PrintHostType::OctoPrint: return std::make_unique<PrintHostOctoPrint>(std::move(config));
        case PrintHostType::PrusaLink: return std::make_unique<PrintHostPrusaLink>(std::move(config));
        case PrintHostType::PrusaLinkStorage: return std::make_unique<PrintHostPrusaLinkStorage>(std::move(config));
        case PrintHostType::SL1Host: return std::make_unique<PrintHostSL1Host>(std::move(config));
        case PrintHostType::Moonraker: return std::make_unique<PrintHostMoonraker>(std::move(config));
        case PrintHostType::Duet: return std::make_unique<PrintHostDuet>(std::move(config));
        case PrintHostType::FlashAir: return std::make_unique<PrintHostFlashAir>(std::move(config));
        case PrintHostType::AstroBox: return std::make_unique<PrintHostAstroBox>(std::move(config));
        case PrintHostType::Repetier: return std::make_unique<PrintHostRepetier>(std::move(config));
        case PrintHostType::MKS: return std::make_unique<PrintHostMKS>(std::move(config));
        case PrintHostType::PrusaConnect: return std::make_unique<PrintHostPrusaConnect>(std::move(config));
        default: ASSERT(false); return nullptr;
     }
    return nullptr;
}

} // Slic3r::Biz::PrintHost 