///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/App/Render/Geometry.hpp>

#include <cstddef>
#include <memory>

namespace Slic3r::App::Render {
class Device;
class Material;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {
class NodeBuilder;
class Scene;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::libvgcode {

class SegmentTemplate
{
public:
    SegmentTemplate() = default;
    ~SegmentTemplate() = default;
    SegmentTemplate(const SegmentTemplate&) = delete;
    SegmentTemplate(SegmentTemplate&&) = delete;
    SegmentTemplate& operator = (const SegmentTemplate&) = delete;
    SegmentTemplate& operator = (SegmentTemplate&&) = delete;

    void init(Render::Device& device, Scene::NodeBuilder& builder);

private:
    std::unique_ptr<Render::Geometry> m_geometry;
};

} // namespace Slic3r::App::libvgcode
