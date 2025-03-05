#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp> // для матчеров в CHECK_THAT
#include <catch2/matchers/catch_matchers_floating_point.hpp> // для Catch::Matchers::WithinRel

#include <catch2/matchers/catch_matchers_templated.hpp> // для собственного мэтчера EqualPosition

#include "../src/geom.h"
#include "../src/model.h"

using Catch::Matchers::WithinRel;
using namespace model;

constexpr double EPSILON = 1e-10;

struct EqualPosition : Catch::Matchers::MatcherGenericBase {
    EqualPosition(geom::Point2D pos) {
        pos_.x = pos.x;
        pos_.y = pos.y;
    }

    EqualPosition(EqualPosition&&) = default;

    bool match(geom::Point2D other) const {
        return std::abs(other.x - pos_.x) < EPSILON &&
               std::abs(other.y - pos_.y) < EPSILON;

    }

    std::string describe() const override {
        // Описание свойства, проверяемого матчером:
        return "Invalid geom::Point2D: {" + std::to_string(pos_.x) + ", " + std::to_string(pos_.y) + "}";
    }

private:
    geom::Point2D pos_;
}; 

SCENARIO("Map with given speed and loot parameters", "[model::Map]") {
    GIVEN("A Map by ID = id_1 and name = Map_1") {
        Map map(Map::Id{"id_1"}, "Map_1");

        THEN("The map has an ID and a name") {
            CHECK(&map.GetId() != nullptr);
            CHECK(*map.GetId() == "id_1");
            CHECK(map.GetName() == "Map_1");

            THEN("The map doesn't have any loot types yet") {
                CHECK(map.GetLootTypesCount() == 0);
                CHECK(map.GetLootTypes().empty());

                THEN("Dog's speed is not set") {
                    CHECK(map.GetSpeed() == std::nullopt);
                }
            }
        }

        WHEN("Set dog speed 2.5") {
            map.SetDogSpeed(2.5);

            THEN("Dog's speed is 2.5") {
                CHECK(map.GetSpeed() == 2.5);
            }
        }

        WHEN("Set dog speed 2.0") {
            map.SetDogSpeed(2.0);

            THEN("Dog's speed is 2.0") {
                CHECK(map.GetSpeed() == 2.0);
            }
        }

        WHEN("Add a first LootType") {
            LootType loot_type;
            loot_type.properties["int_number"] = 7;
            loot_type.properties["bool_true"] = true;
            map.AddLootType(loot_type);

            THEN("Can get the first LootType") {
                CHECK(!map.GetLootTypes().empty());
                CHECK(map.GetLootTypesCount() == 1);
                CHECK(map.GetLootTypes()[0].properties.size() == 2);
                CHECK(std::get<int64_t>(map.GetLootTypes()[0].properties.at("int_number")) == 7);
                CHECK(std::get<bool>(map.GetLootTypes()[0].properties.at("bool_true")) == true);

                WHEN("Add a second LootType") {
                    LootType loot_type;
                    loot_type.properties["double_number"] = 8.5;
                    loot_type.properties["string"] = "name_of_loot";
                    loot_type.properties["int_number"] = 19;
                    map.AddLootType(loot_type);

                    THEN("Can get the second LootType") {
                        CHECK(!map.GetLootTypes().empty());
                        CHECK(map.GetLootTypesCount() == 2);
                        CHECK(map.GetLootTypes()[1].properties.size() == 3);
                        CHECK(std::get<int64_t>(map.GetLootTypes()[1].properties.at("int_number")) == 19);
                        CHECK(std::get<double>(map.GetLootTypes()[1].properties.at("double_number")) == 8.5);
                        CHECK(std::get<std::string>(map.GetLootTypes()[1].properties.at("string")) == "name_of_loot");
                    }
                }
            }
        }

        GIVEN("Define roads") {
            Road road1 = Road(Road::HORIZONTAL, Point{0, 0}, Coord{5});
            Road road2 = Road(Road::VERTICAL, Point{2, 2}, Coord{7});

            WHEN("Add roads") {
                map.AddRoad(road1);
                map.AddRoad(road2);

                THEN("Check the presence of roads") {
                    CHECK(!map.GetRoads().empty());
                    CHECK(map.GetRoads().size() == 2);
                }

                THEN("Check the types of roads") {
                    CHECK(map.GetRoads()[0]->IsHorizontal());
                    CHECK(map.GetRoads()[1]->IsVertical());

                    CHECK(!map.GetRoads()[0]->IsVertical());
                    CHECK(!map.GetRoads()[1]->IsHorizontal());
                }

                GIVEN("Define pointers to both roads") {
                    auto road1_ptr = map.FindRoadByPositionAndDirection(geom::Point2D{2, 0}, Direction::WEST);
                    auto road2_ptr = map.FindRoadByPositionAndDirection(geom::Point2D{2, 4}, Direction::NORTH);

                    THEN("Check the presence of road") {
                        CHECK(road1_ptr != nullptr);
                        AND_THEN("Verify that it is road1 by the known position {2, 0}") {
                            CHECK(road1_ptr->IsPositionOnRoad(geom::Point2D{2, 0}));
                        }
                        AND_THEN("The point {5.4, 0.4}, lying on the road boundary, should belong to the road - corner") {
                            CHECK(road1_ptr->IsPositionOnRoad(geom::Point2D{5.4, 0.4}));
                        }
                        AND_THEN("The point {-0.4, -0.4}, lying on the road boundary, should belong to the road - corner") {
                            CHECK(road1_ptr->IsPositionOnRoad(geom::Point2D{-0.4, -0.4}));
                        }
                        AND_THEN("The point {5.4, -0.4}, lying on the road boundary, should belong to the road - corner") {
                            CHECK(road1_ptr->IsPositionOnRoad(geom::Point2D{5.4, -0.4}));
                        }
                        AND_THEN("The point {-0.4, 0.4}, lying on the road boundary, should belong to the road - corner") {
                            CHECK(road1_ptr->IsPositionOnRoad(geom::Point2D{-0.4, 0.4}));
                        }
                        AND_THEN("The point {0.5, 0.6} does not belong to the road") {
                            CHECK(!road1_ptr->IsPositionOnRoad(geom::Point2D{0.5, 0.6}));
                        }
                    }

                    THEN("Check the presence of road") {
                        CHECK(road2_ptr != nullptr);
                        AND_THEN("Verify that it is road2 by the known position {2, 4}") {
                            CHECK(road2_ptr->IsPositionOnRoad(geom::Point2D{2, 4}));
                        }
                    }

                    THEN("Verify that roads road1 and road2 do not intersect") {
                        CHECK(!road1_ptr->IsIntersect(road2_ptr));
                        CHECK(!road2_ptr->IsIntersect(road1_ptr));
                    }

                    GIVEN("A random road") {
                        auto random_road_ptr = map.GetRandomRoad();
                        THEN("Check the selection of a random road") {
                            CHECK((random_road_ptr == road1_ptr || random_road_ptr == road2_ptr));
                        }
                    }
                }
            } 
        }
    }
}


SCENARIO("Game Session with two players dogs", "[model::GameSession]") {
    GIVEN("A first map by ID = id_1 and name = Map_1") {
        Map map1(Map::Id{"id_1"}, "Map_1");

        GIVEN("Create a game session for the first map with loot parameters: period 5s and probability 0.5") {
           GameSession gamesession1(GameSession(&map1, 5.0, 0.5));

            THEN("The map doesn't have any dogs and loots") {
                CHECK(gamesession1.GetLoots().empty());
                CHECK(gamesession1.GetDogs().empty());
                CHECK(gamesession1.GetLootsCount() == 0);
                CHECK(gamesession1.GetDogsCount() == 0);
            }

            GIVEN("Two players dogs") {
                DogPtr dog1(std::make_shared<Dog>(Dog::Id{0}, "Icy", geom::Point2D{-0.3, -0.4}, 4.5, 3));
                DogPtr dog2(std::make_shared<Dog>(Dog::Id{1}, "Wolfy", geom::Point2D{1.1, 5.5}, 5.0, 3));


                WHEN("Let's add two players dogs") {
                    gamesession1.AddDog(dog1);
                    gamesession1.AddDog(dog2);

                    THEN("Two players dogs have been added") {
                        CHECK(!gamesession1.GetDogs().empty());
                        CHECK(gamesession1.GetDogsCount() == 2);
                        CHECK(gamesession1.GetDog(Dog::Id{0}) != nullptr);
                        CHECK(gamesession1.GetDog(Dog::Id{1}) != nullptr);
                        CHECK(gamesession1.GetDog(Dog::Id{2}) == nullptr);

                        CHECK(*gamesession1.GetDog(Dog::Id{0})->GetId() == 0);
                        CHECK(*gamesession1.GetDog(Dog::Id{1})->GetId() == 1);

                        CHECK(gamesession1.GetDog(Dog::Id{0})->GetName() == "Icy");
                        CHECK(gamesession1.GetDog(Dog::Id{1})->GetName() == "Wolfy");

                        CHECK_THAT(gamesession1.GetDog(Dog::Id{0})->GetPosition(), EqualPosition(geom::Point2D{-0.3, -0.4}));
                        CHECK_THAT(gamesession1.GetDog(Dog::Id{1})->GetPosition(), EqualPosition(geom::Point2D{1.1, 5.5}));

                        CHECK_THAT(gamesession1.GetDog(Dog::Id{0})->GetDefaultSpeed(), WithinRel(4.5, EPSILON));
                        CHECK_THAT(gamesession1.GetDog(Dog::Id{1})->GetDefaultSpeed(), WithinRel(5.0, EPSILON));

                        AND_THEN("But there shouldn't be any loot on the map yet") {
                            CHECK(gamesession1.GetLoots().empty());
                            CHECK(gamesession1.GetLootsCount() == 0);

                            GIVEN("0 loots count for two dogs") {
                                auto loot_count = 0;
                                auto looter_count = 2;
                                
                                WHEN("Generate new loots - low probability of generation") {
                                    auto new_loot_count = gamesession1.GetLootGenerator()->Generate(std::chrono::milliseconds(1), loot_count, looter_count);

                                    THEN("Loots count <= looter count (2)") {
                                        CHECK(new_loot_count + loot_count <= 1);

                                        WHEN("Generate new loots - guaranteed generation") {
                                            auto new_loot_count = gamesession1.GetLootGenerator()->Generate(std::chrono::milliseconds(100000), loot_count, looter_count);

                                            THEN("Loots count <= looter count (2)") {
                                                CHECK(new_loot_count + loot_count == 2);
                                            }
                                        }

                                    }
                                }
                            }

                            GIVEN("1 loot for two dogs") {
                                auto loot_count = 1;
                                auto looter_count = 2;
                                
                                WHEN("Generate new loots - guaranteed generation") {
                                    auto new_loot_count = gamesession1.GetLootGenerator()->Generate(std::chrono::milliseconds(100000), loot_count, looter_count);

                                    THEN("Loots count <= looter count (2)") {
                                        CHECK(new_loot_count + loot_count == 2);
                                    }
                                }
                            }

                            GIVEN("2 loots for two dogs") {
                                auto loot_count = 2;
                                auto looter_count = 2;
                                
                                WHEN("Generate new loots - guaranteed generation") {
                                    auto new_loot_count = gamesession1.GetLootGenerator()->Generate(std::chrono::milliseconds(100000), loot_count, looter_count);

                                    THEN("Loots count <= looter count (2)") {
                                        CHECK(new_loot_count + loot_count == 2);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}