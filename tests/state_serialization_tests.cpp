#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

#include "../src/model.h"
#include "../src/model_serialization.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "double Point2D serialization") {
    GIVEN("a point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "int Point serialization") {
    GIVEN("a point") {
        const model::Point p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                model::Point restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Loot serialization") {
    GIVEN("a loot") {
        const model::Loot l{model::Loot::Id{75}, 9, {12.5, 19.4}};
        WHEN("loot is serialized") {
            output_archive << l;

            THEN("it is equal to loot after serialization") {
                InputArchive input_archive{strm};
                model::Loot restored_loot;
                input_archive >> restored_loot;
                CHECK(l.id == restored_loot.id);
                CHECK(l.type == restored_loot.type);
                CHECK(l.pos == restored_loot.pos);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        Dog dog = [] {
            // параметры Dog(Id id, std::string name, geom::Point2D pos, double default_speed, size_t bag_capacity)
            Dog dog = Dog(Dog::Id{42}, "Pluto"s, {42.2, 12.5}, 3, 4); // предыдущая позиция, 4 предмета возможны
            dog.IncreaseScore(42);
            Loot loot1 = {Loot::Id{5}, 2, {5.4, 2.0}};
            dog.AddLootIntoBag(std::move(std::make_shared<Loot>(loot1)));
            Loot loot2 = {Loot::Id{9}, 1, {0.6, -0.4}};
            dog.AddLootIntoBag(std::move(std::make_shared<Loot>(loot2)));
            dog.SetDirectionSpeed(model::DirectionToString(Direction::EAST));
            dog.SetSpeed({2.3, -1.2});
            dog.SetPosition({42.6, 12.5}); // новая позиция
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr(dog);
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog.GetDefaultSpeed() == restored.GetDefaultSpeed());
                CHECK(dog.GetPosition() == restored.GetPosition());
                CHECK(dog.GetPrevPosition() == restored.GetPrevPosition());
                CHECK(dog.GetSpeed() == restored.GetSpeed());
                CHECK(dog.GetBagCapacity() == restored.GetBagCapacity());
                CHECK(dog.GetScore() == restored.GetScore());
                CHECK(dog.GetDirection() == restored.GetDirection());
                for (size_t i = 0; i < dog.GetBag().size(); ++i) {
                    CHECK(*(dog.GetBag()[i]) == *(restored.GetBag()[i]));
                }
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "GameSession Serialization") {
    GIVEN("a gamesession") {
        Map map(Map::Id{"id_1"}, "Map_1");
        GameSession session = [&map] {
            GameSession session = GameSession(&map, 5.0, 0.5);
            DogPtr dog1(std::move(std::make_shared<Dog>(Dog::Id{0}, "Icy", geom::Point2D{-0.3, -0.4}, 4.5, 3)));
            DogPtr dog2(std::move(std::make_shared<Dog>(Dog::Id{1}, "Wolfy", geom::Point2D{1.1, 5.5}, 5.0, 3)));
            Loot loot1 = {Loot::Id{5}, 2, {5.4, 2.0}};
            dog1->AddLootIntoBag(std::move(std::make_shared<Loot>(loot1)));
            Loot loot2 = {Loot::Id{9}, 1, {0.6, -0.4}};
            dog2->AddLootIntoBag(std::move(std::make_shared<Loot>(loot2)));
            session.AddDog(dog1);
            session.AddDog(dog2);
            return session;
        }();

        WHEN("gamesession is serialized") {
            {
                serialization::GameSessionRepr repr(session);
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::GameSessionRepr repr;
                input_archive >> repr;
                Game game;
                Map map(Map::Id{"id_1"}, "Map_1");
                game.AddMap(map);
                game.SetLootConfig(5.0, 0.5);
                const auto restored = repr.Restore(game);
                CHECK(session.GetDogsCount() == restored.GetDogsCount());
                CHECK(session.GetLootsCount() == restored.GetLootsCount());
                for (size_t i = 0; i < session.GetDogsCount(); ++i) {
                    CHECK(*session.GetDogs()[i]->GetId() == *restored.GetDogs()[i]->GetId());
                    CHECK(session.GetDogs()[i]->GetName() == restored.GetDogs()[i]->GetName());
                    CHECK(session.GetDogs()[i]->GetBagCapacity() == restored.GetDogs()[i]->GetBagCapacity());
                    CHECK(session.GetDogs()[i]->GetPosition() == restored.GetDogs()[i]->GetPosition());
                    CHECK(session.GetDogs()[i]->GetDefaultSpeed() == restored.GetDogs()[i]->GetDefaultSpeed());
                    for (size_t j = 0; j < session.GetDogs()[i]->GetBag().size(); ++j) {
                        CHECK(*(session.GetDogs()[i]->GetBag()[j]) == *(restored.GetDogs()[i]->GetBag()[j]));
                    }
                }
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Game Serialization") {
    GIVEN("a game") {
        Map map1(Map::Id{"id_1"}, "Map_1");
        Map map2(Map::Id{"id_2"}, "Map_2");
        Game game = [&map1, &map2] {
            GameSession session1 = GameSession(&map1, 5.0, 0.5);
            GameSession session2 = GameSession(&map2, 6.0, 0.2);
            DogPtr dog1(std::move(std::make_shared<Dog>(Dog::Id{0}, "Icy", geom::Point2D{-0.3, -0.4}, 4.5, 3)));
            DogPtr dog2(std::move(std::make_shared<Dog>(Dog::Id{1}, "Wolfy", geom::Point2D{1.1, 5.5}, 5.0, 3)));
            Loot loot1 = {Loot::Id{5}, 2, {5.4, 2.0}};
            dog1->AddLootIntoBag(std::move(std::make_shared<Loot>(loot1)));
            Loot loot2 = {Loot::Id{9}, 1, {0.6, -0.4}};
            dog2->AddLootIntoBag(std::move(std::make_shared<Loot>(loot2)));
            session1.AddDog(dog1);
            session1.AddDog(dog2);
            Game game;
            game.AddMap(map1);
            game.AddMap(map2);
            game.AddSession(std::make_shared<GameSession>(session1));
            game.AddSession(std::make_shared<GameSession>(session2));
            return game;
        }();

        WHEN("game is serialized") {
            {
                serialization::GameRepr repr(game);
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::GameRepr repr;
                input_archive >> repr;
                Game restored_game; // по ссылке, так как объект уже сордержит данные из конфига и нужно добавить собак и предметы
                Map map1(Map::Id{"id_1"}, "Map_1");
                restored_game.AddMap(map1);
                Map map2(Map::Id{"id_2"}, "Map_2");
                restored_game.AddMap(map2);
                restored_game.SetLootConfig(5.0, 0.5);
                repr.Restore(restored_game);
                CHECK(game.GetSessionsCount() == restored_game.GetSessionsCount());
                CHECK(game.FindMap(Map::Id{"id_1"})->GetName() == restored_game.FindMap(Map::Id{"id_1"})->GetName());
                CHECK(game.FindMap(Map::Id{"id_2"})->GetName() == restored_game.FindMap(Map::Id{"id_2"})->GetName());
                CHECK(game.FindMap(Map::Id{"id_X"}) == restored_game.FindMap(Map::Id{"id_X"})); // nullptr
                auto game_sessions = game.GetSessions();
                auto restored_game_sessions = restored_game.GetSessions();
                for (size_t i = 0; i < game.GetSessionsCount(); ++i) {
                    auto session = game_sessions[i];
                    auto restored_session = restored_game_sessions[i];
                    for (size_t j = 0; j < session->GetDogsCount(); ++j) {
                        auto dog = session->GetDogs()[j];
                        auto restored_dog = restored_session->GetDogs()[j];
                        CHECK(*dog->GetId() == *restored_dog->GetId());
                        CHECK(dog->GetName() == restored_dog->GetName());
                        CHECK(dog->GetBagCapacity() == restored_dog->GetBagCapacity());
                        CHECK(dog->GetPosition() == restored_dog->GetPosition());
                        CHECK(dog->GetDefaultSpeed() == restored_dog->GetDefaultSpeed());
                        for (size_t k = 0; k < dog->GetBag().size(); ++k) {
                            CHECK(*(dog->GetBag()[k]) == *(restored_dog->GetBag()[k]));
                        }
                    }
                }
            }
        }
    }
}