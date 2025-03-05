#pragma once

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

#include "geom.h"
#include "model.h"


namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Point& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Loot& loot, [[maybe_unused]] const unsigned version) {
    ar& *loot.id;
    ar& loot.pos;
    ar& loot.type;
}

}  // namespace model

namespace serialization {

using namespace model;

// LootRepr (LootsRepresentation) - сериализованное представление класса Loot
class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const Loot& loot)
        : id_(loot.id)
        , type_(loot.type)
        , pos_(loot.pos) {
    }

    [[nodiscard]] Loot Restore() const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& type_;
        ar& pos_;
    }

    Loot::Id id_ = Loot::Id{0u};
    int type_;
    geom::Point2D pos_;
    int value_;
};

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {

public:
    DogRepr() = default;

    explicit DogRepr(const Dog& dog)
        : id_(dog.GetId()) // Id id_
        , name_(dog.GetName()) // std::string name_
        , pos_(dog.GetPosition()) // geom::Point2D pos_
        , default_speed_(dog.GetDefaultSpeed()) // double default_speed_
        , bag_capacity_(dog.GetBagCapacity()) // size_t bag_capacity_
        , prev_pos_(dog.GetPrevPosition()) // geom::Point2D prev_pos_
        , speed_(dog.GetSpeed()) // geom::Vec2D speed_ 
        , dir_(dog.GetDirection()) // Direction dir_ size_t score_
        , score_(dog.GetScore()) // size_t score_
        , bag_(dog.GetBag())  // LootPtrs bag_;
        , play_time_(dog.GetPlayTime()) // ms
        , inactivity_time_(dog.GetInactivityTime()) // ms
        , is_moving_(dog.IsMoving()) {
    }

    [[nodiscard]] Dog Restore() const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& name_;
        ar& pos_;
        ar& default_speed_;
        ar& bag_capacity_;
        ar& prev_pos_;
        ar& speed_;
        ar& dir_;
        ar& score_;
        ar& bag_;
        ar& play_time_;
        ar& inactivity_time_;
        ar& is_moving_;
    }

    Dog::Id id_ = Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    double default_speed_;
    size_t bag_capacity_ = 0;
    geom::Point2D prev_pos_;
    geom::Vec2D speed_;
    Direction dir_ = Direction::NORTH;
    size_t score_ = 0;
    LootPtrs bag_;
    uint32_t play_time_ = 0; // ms
    uint32_t inactivity_time_ = 0; // ms
    bool is_moving_ = false;
};

// Представление GameSession
class GameSessionRepr {
public:
    GameSessionRepr() = default;
    explicit GameSessionRepr(const GameSession& session)
        : id_(session.GetMap()->GetId()) {
        for (DogPtr dog : session.GetDogs()) {
            dogs_.emplace_back(*dog);
        }
        for (LootPtr loot: session.GetLoots()) {
            loots_.emplace_back(*loot);
        }
    }

    GameSession Restore(const Game& game) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & *id_;
        ar & dogs_;
        ar & loots_;
    }

    Map::Id id_ = Map::Id{""};
    std::vector<DogRepr> dogs_;
    std::vector<LootRepr> loots_;
};

// Представление Game
class GameRepr {
public:
    GameRepr() = default;
    explicit GameRepr(const Game& game) {
        for (const auto& session : game.GetSessions()) {
            sessions_.emplace_back(*session);
        }
        total_loot_count_ = game.GetTotalLootsCount();
        total_dog_count_ = game.GetTotalDogsCount();
    }

    void Restore(Game& game) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
        ar & sessions_;
        ar & total_loot_count_;
        ar & total_dog_count_;
    }

    std::vector<GameSessionRepr> sessions_;
    uint32_t total_loot_count_ = 0;
    uint32_t total_dog_count_ = 0;
};

}  // namespace serialization
