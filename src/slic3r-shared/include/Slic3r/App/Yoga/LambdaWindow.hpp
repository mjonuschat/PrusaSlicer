#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {

class LambdaItem;

class LambdaWindow : public Window
{
public:
    explicit LambdaWindow(RenderPosFn render_fn, const std::string& prefix, Item* parent);
    ~LambdaWindow();

    void render_body(Vec2f pos, Vec2f size) override;

private:
    LambdaItem* m_lambda_item = nullptr;
};

}
