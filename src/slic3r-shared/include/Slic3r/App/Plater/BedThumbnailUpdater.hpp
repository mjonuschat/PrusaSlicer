#pragma once

#include "Slic3r/App/Plater/IBedVisuallyChangedListener.hpp"

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App {
class ObjectListWindow;
struct BedThumbnailStore;
} // namespace Slic3r::App

namespace Slic3r::App::Plater {

class BedThumbnailTextureGenerator;

class BedThumbnailUpdater : public IBedVisuallyChangedListener
{
public:
    BedThumbnailUpdater(Render::Device& device, BedThumbnailTextureGenerator& thumbnail_generator, ObjectListWindow& object_list,
        BedThumbnailStore& store)
        : m_device(device), m_thumbnail_generator(thumbnail_generator), m_object_list(object_list), m_store(store) {}

    /**
     * @name Implementation of IBedVisuallyChangedListener public interface
     * @{
     */
    void on_bed_changed(Domain::SelectionId project_id, const Domain::BedRefs& bed_refs) override;
    /**@}*/

private:
    Render::Device& m_device;
    BedThumbnailTextureGenerator& m_thumbnail_generator;
    ObjectListWindow& m_object_list;
    BedThumbnailStore& m_store;
};

} // namespace Slic3r::App::Plater