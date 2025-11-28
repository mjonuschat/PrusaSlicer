#pragma once


#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {
    class InputTextField;
    class DoubleValidator;
}

namespace Slic3r::App::Plater {

class TripleInput : public Yoga::Item
{
public:
    struct Header {
        std::string text;
        ImColor color;
    };

    TripleInput(const std::string& suffix, const std::vector<Header>& header = {});
    std::function<void(const Domain::Vec3d&, int)> on_change = [](const Domain::Vec3d&, int index) {};

    Domain::Vec3d get_value() const;
    void set_value(const Domain::Vec3d& value);

private:
    std::array<Yoga::InputTextField*, 3> m_input;
    std::array<Yoga::DoubleValidator*, 3> m_validator;
};
} // namespace Slic3r::App::Plater
