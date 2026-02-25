#include "Slic3r/Biz/PresetUpdater/TestPresetUpdaterListener.hpp"

#include <nlohmann/json.hpp>
#include "Slic3r/Assert.hpp"
#include <catch2/catch_test_macros.hpp>

namespace Slic3r::Biz::PresetUpdater {
void to_json(nlohmann::json& j, const PresetUpdaterWarning& w) {
    j = nlohmann::json{
        {"text", w.text},
        {"repo", w.repo},
        {"vendor", w.vendor}
    };
}

void from_json(const nlohmann::json& j, PresetUpdaterWarning& w) {
    j.at("text").get_to(w.text);
    j.at("repo").get_to(w.repo);
    j.at("vendor").get_to(w.vendor);
}
}

namespace Slic3r::Biz::PresetUpdater {

TestPresetUpdaterListener::TestPresetUpdaterListener(
    PresetUpdaterInteractor& preset_updater_interactor
) :
    m_preset_updater_interactor(preset_updater_interactor)
{
    m_preset_updater_interactor.add_listener<IPresetUpdaterResultListener>(
        this
    );
}

void TestPresetUpdaterListener::start(ExpectedReconfiguration expected) 
{
    m_has_result = false;
    m_expected = std::move(expected);
    m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(true);
}

void TestPresetUpdaterListener::on_preset_updater_error(const std::string& body)
{
    if (m_expected.state != ReconfigurationResult::Error) {
         printf("Error: %s\n", body.c_str());
    }
    REQUIRE (m_expected.state == ReconfigurationResult::Error);
    m_has_result = true;

}

void TestPresetUpdaterListener::on_preset_updater_reconfigurations_list(
    const PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<PresetUpdaterWarning>& warnings
)
{
    
    if (!warnings.empty()) {
        // This is being commented out due to too much noise in the channel.
        // It may potentinally hide some problems, but it should be generally ok to hide.
        //nlohmann::json j = warnings;
        //printf(j.dump(4).c_str());
        //printf("\n");
    }
    if (!m_expected.allow_warnings)
    {
        REQUIRE(warnings.empty());
    }
    switch (m_expected.state)
    {
    case ReconfigurationResult::None:
        CHECK(reconfigurations.forced_downgrades().empty());
        CHECK(reconfigurations.forced_updates().empty());
        CHECK(reconfigurations.new_vendors().empty());
        CHECK(reconfigurations.not_in_index().empty());
        CHECK(reconfigurations.regular_updates().empty());
        break;
    case ReconfigurationResult::Update:
        CHECK(reconfigurations.forced_downgrades().empty());
        CHECK(reconfigurations.forced_updates().empty());
        CHECK(reconfigurations.new_vendors().empty());
        CHECK(reconfigurations.not_in_index().empty());

        REQUIRE(reconfigurations.regular_updates().size() == 1);
        CHECK(reconfigurations.regular_updates().front().recommended_version == m_expected.version);
        break;

    case ReconfigurationResult::ForcedUpdate:
        CHECK(reconfigurations.forced_downgrades().empty());
        CHECK(reconfigurations.regular_updates().empty());
        CHECK(reconfigurations.new_vendors().empty());
        CHECK(reconfigurations.not_in_index().empty());

        REQUIRE(reconfigurations.forced_updates().size() == 1);
        CHECK(reconfigurations.forced_updates().front().recommended_version == m_expected.version);
        break;

    case ReconfigurationResult::ForcedDowngrade:
        REQUIRE(reconfigurations.regular_updates().empty());
        REQUIRE(reconfigurations.forced_updates().empty());
        REQUIRE(reconfigurations.new_vendors().empty());
        REQUIRE(reconfigurations.not_in_index().empty());

        REQUIRE(reconfigurations.forced_downgrades().size() == 1);
        CHECK(reconfigurations.forced_downgrades().front().recommended_version == m_expected.version);
        break;
    
    case ReconfigurationResult::NotInIndex:
        CHECK(reconfigurations.regular_updates().empty());
        CHECK(reconfigurations.forced_updates().empty());
        CHECK(reconfigurations.new_vendors().empty());
        CHECK(reconfigurations.forced_downgrades().empty());

        REQUIRE(reconfigurations.not_in_index().size() == 1);
        CHECK(reconfigurations.not_in_index().front().recommended_version == m_expected.version);
        break;

    case ReconfigurationResult::NewVendor:
        CHECK(reconfigurations.regular_updates().empty());
        CHECK(reconfigurations.forced_updates().empty());
        CHECK(reconfigurations.not_in_index().empty());
        CHECK(reconfigurations.forced_downgrades().empty());

        REQUIRE(reconfigurations.new_vendors().size() == 1);
        CHECK(reconfigurations.new_vendors().front().recommended_version == m_expected.version);
        break;
    default:
        ASSERT(false);
        break;
    }
    m_has_result = true;
}

void TestPresetUpdaterListener::on_preset_updater_reconfigurations_perfomed(
    const std::vector<PresetUpdaterWarning>& warnings
)
{}

void TestPresetUpdaterListener::on_preset_updater_status(
    const std::string& target,
    int attempt,
    unsigned delay
)
{}

void TestPresetUpdaterListener::on_preset_updater_repository_info_vector(
    const SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<PresetUpdaterWarning>& warnings
)
{}

} // namespace Slic3r::Biz::PresetUpdater
