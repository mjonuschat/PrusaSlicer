///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/PrintToolItem.hpp"
#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

#include "Slic3r/App/IConfigNavigable.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Config/ToolRowControl.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"

namespace Slic3r::Biz {
class PrintToolConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigRowItem;
class PrintToolRowButton;

using ExplanationPart = std::pair<std::string, ImColor>;

class ExplanationView : public Biz::DataObserver<ExplanationPart>, public Yoga::Text
{
public:
    ExplanationView(size_t index, const ExplanationPart& data);

protected:
    void on_data_update() override;
};

class PrintToolRowItem :
    public Biz::DataObserver<Biz::PrintToolItem>,
    public IConfigNavigable,
    public Yoga::Rectangle,
    public Biz::IObservableList<ToolRowOverride>
{
public:
    PrintToolRowItem(
        size_t index,
        const Biz::PrintToolItem& data,
        Biz::PrintToolConfigBoxInteractor& cbi,
        Biz::IConfigBoxSetter& cbi_setter
    );
    ~PrintToolRowItem() override;

    void navigate_to_item(const Domain::ConfigItem* config_item) override;
    void clear_navigation() override;

    const ToolRowOverride& at(size_t index) const override;
    size_t size() const override;

protected:
    void on_data_update() override;

    void clear();
    void initialize();
    void update_explanation();

private:
    using ToolRowFactory =
        Yoga::ViewFactory<ToolRowControl, ToolRowOverride, Biz::IConfigBoxSetter&>;
    using ToolRowListView = Yoga::ListView<ToolRowControl, ToolRowOverride, ToolRowFactory>;

    using ExplanationListView = Yoga::ListView<ExplanationView, ExplanationPart>;

    enum class InitializedType
    {
        None,
        PrintOnly,
        PrintTool
    };
    InitializedType m_initialized_type{InitializedType::None};

    Biz::PrintToolConfigBoxInteractor& m_cbi;
    Biz::IConfigBoxSetter& m_cbi_setter;

    PrintToolRowButton* m_main_button{nullptr};
    Yoga::Rectangle* m_content{nullptr};
    Yoga::Item* m_explanation_container{nullptr};
    ToolRowListView* m_tool_list_view{nullptr};

    Biz::UnsharedPointer<Biz::ObservableList<ExplanationPart>> m_explanation_list_labels;
    Biz::UnsharedPointer<Biz::ObservableList<ExplanationPart>> m_explanation_list;
    ExplanationListView* m_explanation_labels_list_view{nullptr};
    ExplanationListView* m_explanation_list_view{nullptr};
    Yoga::Text* m_explanation_label{nullptr};

    std::vector<ToolRowOverride> m_tool_overrides;

    ConfigRowItem* m_config_row_item{nullptr};
};

} // namespace Slic3r::App
