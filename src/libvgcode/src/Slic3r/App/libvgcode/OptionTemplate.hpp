///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <cstdint>
#include <cstddef>

namespace Slic3r::App::Render {
class Device;
class Material;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {
class NodeBuilder;
class GeometryDataFactory;
class Scene;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::libvgcode {

class OptionTemplate
{
public:
    OptionTemplate() = default;
    ~OptionTemplate() = default;
    OptionTemplate(const OptionTemplate&) = delete;
    OptionTemplate(OptionTemplate&&) = delete;
    OptionTemplate& operator = (const OptionTemplate&) = delete;
    OptionTemplate& operator = (OptionTemplate&&) = delete;

    void init(Render::Device& device, Scene::NodeBuilder& builder, Scene::GeometryDataFactory& data_factory);
};

} // namespace Slic3r::App::libvgcode
