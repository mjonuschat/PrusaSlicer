#include <catch2/catch_test_macros.hpp>
#include <test_utils.hpp>

#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "libslic3r/SLAPrint.hpp"

#include "Slic3r/Domain/ConfigPack.hpp"

#include <boost/filesystem.hpp>

using namespace Slic3r;
using namespace Slic3r::Biz::Slicing; // SLAResult

TEST_CASE("Archive export test", "[sla_archives]") {
    SLAResult sla_result;
    SLAPrint::OnSlaResult on_sla_result = [&sla_result](SLAResult&& r) {
        // only 2 steps of data propagation
        if (!r.files.data.empty()) {
            sla_result.files = std::move(r.files);
        } else {
            sla_result = std::move(r); 
        }
    };
    SLAPrint::OnSlaObject on_sla_object = [](const Sla::Object& i) {
        // nothing to do
    };

    for (const char * pname : {"20mm_cube", "extruder_idler"}){
        INFO("Testing archive type: SL1 -- writing...");
        SLAPrint print(on_sla_result, on_sla_object);

        Domain::Model m;
        ASSERT(load_obj((TEST_DATA_DIR PATH_SEPARATOR + std::string(pname) + ".obj").c_str(), &m));
        m.objects.back()->add_instance();

        Domain::ConfigPackSLA cfg;

        cfg.sla_printer_settings.items.opt("printer_technology").set(Domain::PrinterTechnology::SLA); // FIXME this should be ensured
        cfg.sla_printer_settings.items.opt("sla_archive_format").set("SL1");
        cfg.sla_print_settings.items.opt("supports_enable").set(false);
        cfg.sla_print_settings.items.opt("pad_enable").set(false);


        print.set_status_callback([](const PrintBase::SlicingStatus&) {});

        const Biz::Print::SerializedConfig serialized_config{
            .json = Biz::beautify_json(Domain::as_boxes(cfg), 2),
            .ini = Biz::serialize_as_legacy_config(cfg)
        };
        print.apply(m, cfg, serialized_config);
        print.process();

        auto outputfname = std::string("output_") + pname + ".sl1";

        export_print(outputfname, sla_result, {}, pname);

        // Not much can be checked about the archives...
        REQUIRE(boost::filesystem::exists(outputfname));
    }
}
