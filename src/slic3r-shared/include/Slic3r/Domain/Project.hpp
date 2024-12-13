#pragma once

#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/BedContainer.hpp"

namespace Slic3r {
class Model;
class ModelObject;
class ModelVolume;
class ModelInstance;
enum class ModelVolumeType : int;
}

namespace Slic3r::Domain {

/**
 * All data that can be loaded/saved into .3mf file.
 */
class Project final
{
public:
    using ConfigContainerList = std::vector<std::unique_ptr<ConfigContainer>>;

    Project();

    void load(const std::string& file_path);

    void set_file_name(const std::string& file_name) { m_file_name = file_name; }
    [[nodiscard]] const std::string& file_name() const { return m_file_name; }

    [[nodiscard]] ConfigContainerList& config_containers() { return m_config_containers; }
    [[nodiscard]] const ConfigContainerList& config_containers() const { return m_config_containers; }
    ConfigContainerList::const_iterator find_config_container(size_t id) const
    {
        return std::find_if(
            m_config_containers.begin(), m_config_containers.end(),
            [id](const auto& cc) { return cc->id().id == id; }
        );
    }

    const Model& model() const { return *m_model; }
    Model& model() { return *m_model; }

    const ModelObject* find_object_by_id(size_t id) const;
    const ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id) const;
    const ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id) const;
    ModelObject* find_object_by_id(size_t id);
    ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id);
    ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id);

    const BedContainer& bed_container() const { return m_bed_container; }
    BedContainer& bed_container() { return m_bed_container; }

private:
    std::string m_file_name;
    ConfigContainerList m_config_containers;
    BedContainer m_bed_container;
    std::unique_ptr<Model> m_model;
};

template <typename T, typename C>
T* find_by_id(const C& container, size_t id)
{
    auto it = std::find_if(container.begin(), container.end(), [id](const auto& e) {
        return e->id() == id;
    });
    return it == container.end() ? nullptr : *it;
}

} // namespace Slic3r::Domain