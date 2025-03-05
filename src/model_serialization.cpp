#include "model_serialization.h"


namespace serialization {

// методы класса LootRepr

    [[nodiscard]] Loot LootRepr::Restore() const {
        Loot loot;
        loot.id = id_;
        loot.type = type_;
        loot.pos = pos_;
        return loot;
    }

// методы класса DogRepr

    [[nodiscard]] Dog DogRepr::Restore() const {
        Dog dog = Dog(id_, name_, prev_pos_, default_speed_, bag_capacity_);
        dog.SetPosition(pos_);
        dog.SetDirectionSpeed(DirectionToString(dir_));
        dog.SetSpeed(speed_);
        dog.IncreaseScore(score_);
        for (LootPtr loot : bag_) {
            dog.AddLootIntoBag(std::move(loot));
        }
        dog.SetPlayTime(play_time_);
        dog.SetInactivityTime(inactivity_time_);
        if (is_moving_) {
            dog.Moving();
        } else {
            dog.Stopped();
        }
        return dog;
    }

// методы класса GameSessionRepr

    GameSession GameSessionRepr::Restore(const Game& game) const { // из game берем параметры, загруженные из конфига
        const Map* map = game.FindMap(id_);
        if (!map) {
            throw std::runtime_error("Invalid Map::Id"); // логируем при вызове Restore
        }
        GameSession session{map, game.GetLootPeriod(), game.GetLootProbability()};
        for (const DogRepr& dog_repr : dogs_) {
            session.AddDog(std::make_shared<Dog>(dog_repr.Restore()));
        }
        for (const LootRepr loot_repr : loots_) {
            session.AddLoot(std::make_shared<Loot>(loot_repr.Restore()));
        }
        return session;
    }

// методы класса GameRepr

    void GameRepr::Restore(Game& game) const { // дополняем данные, загруженные из конфига десериализованными параметрами (игровые сессии)
        for (const auto& session_repr : sessions_) {
            auto session = session_repr.Restore(game);
            game.AddSession(std::make_shared<GameSession>(std::move(session)));
        }
        game.SetTotalLootsCount(total_loot_count_);
        game.SetTotalDogsCount(total_dog_count_);
    }

}  // namespace serialization
