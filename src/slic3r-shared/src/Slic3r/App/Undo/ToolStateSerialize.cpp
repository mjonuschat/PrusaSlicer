#include "Slic3r/App/Undo/ToolStateSerialize.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

namespace cereal {

template <class Archive>
void serialize(Archive&, Slic3r::App::Undo::CutGizmoState& tool_state)
{}

template <class Archive>
void serialize(Archive& ar, Slic3r::App::Undo::HeightRangeGizmoState& tool_state)
{
    ar(tool_state.selected_height_range);
}

} // namespace cereal

namespace Slic3r::App::Undo {

SerializedData serialize_tools_state(const ToolsState& tools_state)
{
    std::ostringstream oss;
    cereal::BinaryOutputArchive archive{oss};
    archive(tools_state);

    SerializedData snapshot;
    snapshot.serialized_data = oss.str();
    return snapshot;
}

ToolsState load_serialized_tools_state(const SerializedData& data)
{
    std::istringstream iss(data.serialized_data);
    cereal::BinaryInputArchive archive(iss);

    ToolsState result;
    archive(result);
    return result;
}

} // namespace Slic3r::App::Undo
