///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ImGuiFixture.hpp"
#include "Slic3r/App/Yoga/TestRootItem.hpp"

#include <Slic3r/App/Yoga/ListView.hpp>
#include <Slic3r/Biz/ObservableList.hpp>
#include <Slic3r/Biz/DataObserver.hpp>

#include <Slic3r/App/Yoga/Text.hpp>

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;

struct DummyFactoryContext
{
    int bar = 42;
};

struct DummyState
{
    int foo = 0;
};

class DummyItem : public Item, public Slic3r::Biz::DataObserver<DummyState>
{
public:
    explicit DummyItem(size_t index, const DummyState& state) :
        Slic3r::Biz::DataObserver<DummyState>(index, state)
    {}

    int foo() const
    {
        return m_state->foo;
    }

    bool updated() const
    {
        return m_updated;
    }

protected:
    void on_data_update()
    {
        m_updated = true;
    }

    bool m_updated = false;
};

class DummyItemWithContext : public Item, public Slic3r::Biz::DataObserver<DummyState>
{
public:
    explicit DummyItemWithContext(size_t index, const DummyState& state, DummyFactoryContext* context) :
        Slic3r::Biz::DataObserver<DummyState>(index, state),
        m_context(context)
    {}

    int foo() const
    {
        return m_state->foo;
    }

    DummyFactoryContext* m_context = nullptr;

protected:
    void on_data_update() {}
};

namespace {
void compare_list_view(
    const Slic3r::Biz::ObservableList<DummyState>& list,
    const ListView<DummyItem, DummyState>* view
)
{
    REQUIRE(list.size() == view->item_count());
    for (size_t i = 0; i < list.size(); ++i) {
        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(i))->foo() == list.at(i).foo);
    }
}
} // namespace

TEST_CASE_METHOD(ImGuiFixture, "ListView after ObservableList")
{
    Slic3r::Biz::ObservableList<DummyState> list;

    {
        TestRootItem tree;

        list.reset({{1}, {2}, {3}});

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        tree.process_loop_events();

        REQUIRE(list.size() == 3);
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView before ObservableList")
{
    Slic3r::Biz::ObservableList<DummyState> list;

    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}});
        list.reset({{1}, {2}, {3}});

        tree.process_loop_events();

        REQUIRE(list.size() == 3);
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::insert")
{
    Slic3r::Biz::ObservableList<DummyState> list;

    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        REQUIRE(view->item_count() == 0);

        list.insert({0}, 0);

        REQUIRE(view->item_count() == 1);

        list.insert({1}, 0);

        REQUIRE(view->item_count() == 2);

        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::remove single")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}});

        REQUIRE(view->item_count() == 3);

        list.remove({2});

        tree.process_loop_events();

        REQUIRE(view->item_count() == 2);

        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::remove multiple")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}});

        REQUIRE(view->item_count() == 3);

        list.remove({2});
        list.remove({0});

        tree.process_loop_events();

        REQUIRE(view->item_count() == 1);

        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::remove range")
{
    Slic3r::Biz::ObservableList<DummyState> list;

    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}});

        REQUIRE(view->item_count() == 3);

        list.remove({1, 2});

        tree.process_loop_events();

        REQUIRE(view->item_count() == 1);

        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::remove all")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}});

        REQUIRE(view->item_count() == 3);

        list.remove({0, 2});

        tree.process_loop_events();

        REQUIRE(view->item_count() == 0);

        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::move begin->end")
{
    Slic3r::Biz::ObservableList<DummyState> list;

    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);
        list.reset({{1}, {2}, {3}, {4}, {5}, {6}, {7}});

        REQUIRE(view->item_count() == 7);

        list.move(0, 6);

        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(0))->foo() == 2);
        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(6))->foo() == 1);
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::move begin<-end")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}, {4}, {5}, {6}, {7}});

        REQUIRE(view->item_count() == 7);

        list.move(6, 0);

        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(0))->foo() == 7);
        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(6))->foo() == 6);
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::move same index")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}, {4}, {5}, {6}, {7}});

        REQUIRE(view->item_count() == 7);

        list.move(3, 3);

        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(3))->foo() == 4);
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView ObservableList::update")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        ListView<DummyItem, DummyState>* view = tree.emplace_back<ListView<DummyItem, DummyState>>();
        view->set_source_list(&list);

        list.reset({{1}, {2}, {3}, {4}, {5}, {6}, {7}});

        REQUIRE(view->item_count() == 7);
        compare_list_view(list, view);

        list.set({9}, 3);

        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(3))->foo() == 9);
        REQUIRE(dynamic_cast<DummyItem*>(view->get_item(3))->updated());
        compare_list_view(list, view);
    }
}

TEST_CASE_METHOD(ImGuiFixture, "ListView with context")
{
    Slic3r::Biz::ObservableList<DummyState> list;
    {
        TestRootItem tree;

        list.reset({{1}, {2}, {3}});

        DummyFactoryContext context;

        ListView<DummyItemWithContext, DummyState, ViewFactory<DummyItemWithContext, DummyState, DummyFactoryContext*>>
            view(&context);
        view.set_source_list(&list);

        REQUIRE(list.size() == 3);
        for (size_t i = 0; i < list.size(); ++i) {
            DummyItemWithContext* item = dynamic_cast<DummyItemWithContext*>(view.get_item(i));
            REQUIRE(item->foo() == list.at(i).foo);
            REQUIRE(item->m_context);
            REQUIRE(item->m_context->bar == 42);
        }
    }
}
