#include "Slic3r/App/Undo/BedSelectionStateSerialize.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace cereal {

template <class Archive>
void serialize(Archive& ar, Slic3r::Domain::BedRef& bed_ref)
{
    ar(bed_ref.config_container_id, bed_ref.instance_id);
}

template <class Archive>
void serialize(Archive& ar, Slic3r::App::Undo::BedSelectionState& selection)
{
    ar(selection.selected_beds,
       selection.selected_config_container,
       selection.last_selected_bed,
       selection.mode,
       selection.camera_action_on_selection);
}
} // namespace cereal

namespace Slic3r::App::Undo {

SerializedData serialize_bed_selection_state(const BedSelectionState& value) {
    std::ostringstream oss;
    cereal::BinaryOutputArchive archive{oss};

    archive(value);

    SerializedData snapshot;
    snapshot.serialized_data = oss.str();

    return snapshot;
}

BedSelectionState load_serialized_bed_selection_state(const SerializedData& data) {
    std::istringstream iss(data.serialized_data);
    cereal::BinaryInputArchive archive(iss);

    BedSelectionState result;
    archive(result);
    return result;
}

} // namespace Slic3r::App::Undo
