///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "AbstractViewer.hpp"
#include "SegmentTemplate.hpp"

//#define USE_TEXTURE_BUFFER (1 && SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED)

namespace Slic3r::App::libvgcode {

struct GCodeInputData;

class SlaViewer : public AbstractViewer
{
public:
    SlaViewer();
    ~SlaViewer() = default;
    SlaViewer(const SlaViewer&) = delete;
    SlaViewer(SlaViewer&&) = delete;
    SlaViewer& operator = (const SlaViewer&) = delete;
    SlaViewer& operator = (SlaViewer&&) = delete;

    /**
     * @brief Initialize rendering geometry
     *
     * @param device The current device.
     * @param scene The current scene.
     * @param data_factory The geometry factory.
     */
    void init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory) override;
    //
    // Reset all caches and free gpu memory.
    //
    void reset() override;
    //
    // Setup the viewer content from the given data (support for SLA printers).
    //
    void load(const std::vector<float>& layers_zs, const std::vector<float>& layers_times);
    //
    // Render
    //
    void render() override;

    void set_layers_range(Interval::value_type min, Interval::value_type max) override;
    void set_view_visible_range(Interval::value_type min, Interval::value_type max) override;

    float estimated_time() const override;
    float estimated_time_at(size_t id) const override;
    std::vector<float> layers_estimated_times() const override;

    // TODO
 //   bool export_output() const;

private:
    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;

    size_t m_enabled_segments_count{ 0 };

    void update_view_full_range() override;
    void render_segments(const Domain::Vec3f& camera_position);
};

} // namespace Slic3r::App::libvgcode
