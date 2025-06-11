#include "Slic3r/Biz/Format/ResultLoad3mf.hpp"
using namespace Slic3r;

ResultLoad3mf::ResultLoad3mf(Read3mfIssueType type) : 
    m_issues({Read3mfIssue{type}}),
    m_is_valid(!break_compatibility(m_issues.back()))
{}

ResultLoad3mf::ResultLoad3mf(bool is_model_loaded) {    
    if (!is_model_loaded) {
        m_issues.push_back({Read3mfIssueType::unknown});
        m_is_valid = false;
    }
}

ResultLoad3mf & ResultLoad3mf::add(const Read3mfIssue &issue){
    m_issues.push_back(issue);
    m_is_valid &= !break_compatibility(issue);
    return *this;
}

ResultLoad3mf::operator bool() const {
    return m_is_valid;
}

void ResultLoad3mf::operator+=(const ResultLoad3mf &r) {
    m_issues.insert(m_issues.end(), r.m_issues.begin(), r.m_issues.end());
    m_is_old_3mf |= r.m_is_old_3mf;
    m_is_valid &= r.m_is_valid;
}

void ResultLoad3mf::operator+=(ResultLoad3mf &&r) {
    m_issues.insert(m_issues.end(), 
        std::make_move_iterator(r.m_issues.begin()), 
        std::make_move_iterator(r.m_issues.end()));
    m_is_old_3mf |= r.m_is_old_3mf;
    m_is_valid &= r.m_is_valid;
}

bool ResultLoad3mf::break_compatibility(const Read3mfIssue &issue) {
    // TODO: Change behavior together with independently updated issue messages
    if (issue.type == Read3mfIssueType::unknown)
        return true;

    return false;
}

#include <magic_enum/magic_enum.hpp>
template <> struct magic_enum::customize::enum_range<Read3mfIssueType> {
    static constexpr int max = 473; // 473 unique value to be full text searchable when exceed
};

namespace Slic3r {

std::string create_message(const Read3mfIssues &issues) { 
    if (issues.empty())
        return "There is no issue";

    std::string message;
    for (const Read3mfIssue &issue: issues) {
        std::string issue_name(magic_enum::enum_name(issue.type));
        message += issue_name + "\n  \t " + issue.source + "\n\n ";
    }
    return message;
}

MeshesWithUUID::const_iterator find_by_ptr(
    const MeshesWithUUID &meshes, const std::shared_ptr<const Domain::TriangleMesh> &mesh_ptr
) {
    return std::find_if(meshes.cbegin(), meshes.cend(), [&mesh_ptr](const MeshWithUUID &mesh_uuid) {
        std::shared_ptr<const Domain::TriangleMesh> mesh = mesh_uuid.mesh.lock();
        if (mesh == nullptr)
            return false;
        return mesh_ptr == mesh;
    });
}
VolumesWithUUID::const_iterator find_by_id(const VolumesWithUUID &volumes, size_t volume_id) {
    return std::find_if(volumes.cbegin(), volumes.cend(),
        [volume_id](const VolumeWithUUID &volume_uuid) { 
            return volume_uuid.volume_id == volume_id; 
        });
}
ComponentsWithUUID::const_iterator find_by_id(const ComponentsWithUUID &components, size_t volume_id) {
    return std::find_if(components.cbegin(), components.cend(),
        [volume_id](const ComponentWithUUID &component) { 
            return component.volume_id == volume_id;
        });
}
ObjectsWithUUID::const_iterator find_by_id(const ObjectsWithUUID &objects, size_t object_id) {
    return std::find_if(objects.cbegin(), objects.cend(),
        [object_id](const ObjectWithUUID &object_uuid) { 
            return object_uuid.object_id == object_id; 
        });
}
ItemsWithUUID::const_iterator find_by_id(const ItemsWithUUID &items, size_t instance_id) {
    return std::find_if(items.cbegin(), items.cend(),
        [instance_id](const ItemWithUUID &item) { 
            return item.instance_id == instance_id; 
        });
}

} // namespace Slic3r