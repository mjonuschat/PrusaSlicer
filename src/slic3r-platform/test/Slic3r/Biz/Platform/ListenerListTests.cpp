#include <catch2/catch_test_macros.hpp>
#include "Slic3r/Biz/Platform/ListenerList.hpp"

TEST_CASE("Edit during invocation", "")
{
    class Interface {};
    Interface i1, i2;
    Slic3r::Biz::ListenerList<Interface> listeners;
    listeners.add(&i1);

    listeners.invoke([&](Interface* i) {
        listeners.remove(&i1); // remove itself
        listeners.add(&i2);
    });

    size_t count = 0;
    listeners.invoke([&](Interface* i) {
        CHECK(i == &i2);
        ++count;
    });
    CHECK(count == 1);
}

// Removed listener can't be called (even if removed in the invoke).
TEST_CASE("Deleted listener can't be called", "")
{
    struct Interface { virtual void check() {}; virtual ~Interface() = default; };
    class Deleted : public Interface { void check() override { CHECK(false); } };

    Interface i;
    Deleted d;

    Slic3r::Biz::ListenerList<Interface> listeners;
    listeners.add(&i);
    listeners.add(&d);
    listeners.invoke([&](Interface* i) {
        listeners.remove(&d);
        i->check();
    });
}

