#include <catch2/catch_test_macros.hpp>
#include <test_utils.hpp>

#include "libslic3r/SLAPrint.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "libslic3r/Format/SLAArchiveReader.hpp"
#include "libslic3r/Format/SL1.hpp"
#include "libslic3r/FileReader.hpp"

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
        SLAFullPrintConfig fullcfg;

        auto m = FileReader::load_model(TEST_DATA_DIR PATH_SEPARATOR + std::string(pname) + ".obj");

        fullcfg.printer_technology.setInt(ptSLA); // FIXME this should be ensured
        fullcfg.set("sla_archive_format", "SL1");
        fullcfg.set("supports_enable", false);
        fullcfg.set("pad_enable", false);

        DynamicPrintConfig cfg;
        cfg.apply(fullcfg);

        print.set_status_callback([](const PrintBase::SlicingStatus&) {});
        print.apply(m, cfg, {}, {});
        print.process();

        ThumbnailsList thumbnails;
        auto outputfname = std::string("output_") + pname + ".sl1";

        export_print(outputfname, sla_result, thumbnails, pname);

        // Not much can be checked about the archives...
        REQUIRE(boost::filesystem::exists(outputfname));

        double vol_written = m.mesh().volume();

        if (true) {
            INFO("Testing archive type: SL1 -- reading back...");
            indexed_triangle_set its;
            DynamicPrintConfig cfg;

            try {
                // Leave format_id deliberetaly empty, guessing should always
                // work here.
                import_sla_archive(outputfname, "", its, cfg);
            } catch (...) {
                REQUIRE(false);
            }

            // its_write_obj(its, (outputfname + ".obj").c_str());

            REQUIRE(!cfg.empty());
            REQUIRE(!its.empty());

            double vol_read = Domain::its_volume(its);
            double rel_err  = std::abs(vol_written - vol_read) / vol_written;
            REQUIRE(rel_err < 0.1);
        }
    }
}
