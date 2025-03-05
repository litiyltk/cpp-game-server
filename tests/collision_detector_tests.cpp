#define _USE_MATH_DEFINES
 
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp> // для матчеров в CHECK_THAT
#include <catch2/matchers/catch_matchers_templated.hpp> // для собственного матчера для IsEventEqual

#include <sstream>
#include <vector>
#include <cmath>
 
#include "../src/collision_detector.h"

using namespace collision_detector;
using namespace geom;

constexpr double EPSILON = 1e-10;

// Специализация StringMaker для GatheringEvent
namespace Catch {
template<>
struct StringMaker<GatheringEvent> {
    static std::string convert(GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << ","
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
} // namespace Catch

// Структура для сравнения двух GatheringEvent
template <typename Event>
struct IsEventEqualMatcher : Catch::Matchers::MatcherGenericBase {
    IsEventEqualMatcher(const Event& event)
        : event_(event) {
    }

    IsEventEqualMatcher(IsEventEqualMatcher&&) = default;

    template <typename OtherEvent>
    bool match(const OtherEvent& other) const {
        return event_.gatherer_id == other.gatherer_id &&
           event_.item_id == other.item_id &&
           std::abs(event_.sq_distance - other.sq_distance) < EPSILON &&
           std::abs(event_.time - other.time) < EPSILON;
    }

    std::string describe() const override {
        return "Invalid Event: " + Catch::StringMaker<Event>::convert(event_);
    }

private:
    Event event_;
}; 

template <typename Event>
IsEventEqualMatcher<Event> IsEventEqual(const Event& event) {
    return IsEventEqualMatcher<Event>(event);
}

TEST_CASE("FindGatherEvents detects all collision events in chronological order for one gatherer", "[FindGatherEvents]") {

    std::vector<Item> items = {
        {{2.0, 5.0}, 1.0}, // id0, выше оси +2, радиус 1
        {{0.0, 2.0}, 0.5}, // id1, на оси, радиус 0.5
        {{-3.0, 4.0}, 2.0} // id2, ниже оси -3, радиус 2
    };

    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {0.0, 5.0}, 1.0}, // вдоль оси OY от 0 до 5, радиус 1
    };

    ModelItemGathererProvider provider(items, gatherers);
    REQUIRE(provider.ItemsCount() == 3);
    REQUIRE(provider.GatherersCount() == 1);

    std::vector<GatheringEvent> expected_events = {
        {1, 0, 0.0, 0.4}, // id1
        {2, 0, 9.0, 0.8}, // id2
        {0, 0, 4.0, 1.0} // id0
    };

    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == expected_events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        if (i != 0) {
            CHECK(events[i].time >= events[i-1].time);
        }
        CHECK_THAT(events[i], IsEventEqual(expected_events[i]));
    }
}

TEST_CASE("FindGatherEvents returns events in chronological order for two gatherer", "[FindGatherEvents]") {

    std::vector<Item> items = {
        {{4.0, 0.0}, 1.0},
        {{1.0, 0.0}, 1.0},
        {{3.0, 0.0}, 1.0}
    };

    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {2.0, 0.0}, 1.0}, // вдоль оси OX от 0 до 2, радиус 1
        {{5.0, 0.0}, {0.0, 0.0}, 1.0}, // против оси OX от 5 до 0, радиус 1
    };

    ModelItemGathererProvider provider(items, gatherers);
    REQUIRE(provider.ItemsCount() == 3);
    REQUIRE(provider.GatherersCount() == 2);

    std::vector<GatheringEvent> expected_events = {
        {0, 1, 0.0, 0.2},
        {2, 1, 0.0, 0.4},
        {1, 0, 0.0, 0.5},
        {1, 1, 0.0, 0.8}
    };

    auto events = FindGatherEvents(provider);
    CHECK(events.size() == expected_events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        if (i != 0) {
            CHECK(events[i].time >= events[i-1].time);
        }
        CHECK_THAT(events[i], IsEventEqual(expected_events[i]));
    }
}

TEST_CASE("FindGatherEvents detects two collision events for two Gatherer", "[FindGatherEvents]") {

    std::vector<Item> items = {
        {{1.0, 0.0}, 1.0}
    };

    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {2.0, 0.0}, 1.0}, // вдоль оси OX от 0 до 2, радиус 1
        {{2.0, 0.0}, {0.0, 0.0}, 1.0}, // против оси OX от 2 до 0, радиус 1
    };

    ModelItemGathererProvider provider(items, gatherers);
    REQUIRE(provider.ItemsCount() == 1);
    REQUIRE(provider.GatherersCount() == 2);

    std::vector<GatheringEvent> expected_events = {
        {0, 0, 0.0, 0.5},
        {0, 1, 0.0, 0.5},
    };

    auto events = FindGatherEvents(provider);
    CHECK(events.size() == expected_events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        if (i != 0) {
            CHECK(events[i].time >= events[i-1].time);
        }
        CHECK_THAT(events[i], IsEventEqual(expected_events[i]));
    }
}

TEST_CASE("FindGatherEvents does not detect extra events", "[FindGatherEvents]") {

    std::vector<Item> items = {
        {{10.0, 10.0}, 1.0},
        {{0.0, 10.1}, 1.0},
        {{0.0, -0.1}, 1.0},
        {{5.0, 5.0}, 1.0},
        {{-5.0, -5.0}, 1.0}
    };

    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {0.0, 10.0}, 2.0}
    };

    ModelItemGathererProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);

    REQUIRE(events.empty());
}

TEST_CASE("FindGatherEvents detects all collision in start point", "[FindGatherEvents]") {

    std::vector<Item> items = {
        {{0.0, 0.0}, 1.0},
        {{0.0, -0.5}, 1.0}
    };

    std::vector<Gatherer> gatherers = {
        {{0.0, 0.0}, {2.0, 0.0}, 1.0} // вдоль оси OX от 0 до 2, радиус 1
    };

    ModelItemGathererProvider provider(items, gatherers);
    REQUIRE(provider.ItemsCount() == 2);
    REQUIRE(provider.GatherersCount() == 1);

    std::vector<GatheringEvent> expected_events = {
        {0, 0, 0.0, 0.0},
        {1, 0, 0.25, 0.0},
    };

    auto events = FindGatherEvents(provider);
    CHECK(events.size() == expected_events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        CHECK_THAT(events[i], IsEventEqual(expected_events[i]));
    }
}