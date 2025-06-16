#pragma once

#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/Forward.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/FindById.hpp"

namespace Slic3r::Domain {
class Model;
class ModelInstance;
class ModelVolume;
class ModelObject;
enum class ModelVolumeType : int;

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

    [[nodiscard]] const Domain::Model& model() const { return *m_model; }
    [[nodiscard]] Domain::Model& model() { return *m_model; }

    [[nodiscard]] const ConfigContainer* find_config_container(size_t id) const;
    [[nodiscard]] ConfigContainer* find_config_container(size_t id);

    [[nodiscard]] const Domain::ModelObject* find_object_by_id(size_t id) const;
    [[nodiscard]] const Domain::ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id) const;
    [[nodiscard]] const Domain::ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id) const;
    [[nodiscard]] Domain::ModelObject* find_object_by_id(size_t id);
    [[nodiscard]] Domain::ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id);
    [[nodiscard]] Domain::ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id);

    [[nodiscard]] const BedInstance* find_bed_instance_by_id(size_t id) const;
    [[nodiscard]] BedInstance* find_bed_instance_by_id(size_t id);

    [[nodiscard]] const BedContainer& bed_container() const { return m_bed_container; }
    [[nodiscard]] BedContainer& bed_container() { return m_bed_container; }
    [[nodiscard]] const Bed* find_bed_by_id(size_t id) const;
    [[nodiscard]] Bed* find_bed_by_id(size_t id);

    ModelInstanceList& unplaced_model_instances() { return m_unplaced_model_instances; }
    const ModelInstanceList& unplaced_model_instances() const { return m_unplaced_model_instances; }

private:
    std::string m_file_name;
    ConfigContainerList m_config_containers;
    BedContainer m_bed_container;
    std::unique_ptr<Domain::Model> m_model;
    ModelInstanceList m_unplaced_model_instances;
};

} // namespace Slic3r::Domain