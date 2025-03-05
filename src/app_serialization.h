#pragma once

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <unordered_map>

#include "app.h"
#include "model.h"
#include "model_serialization.h"


namespace serialization {

using namespace app;
using namespace model;

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const Player& player)
        : id_(player.GetDogId()) // Id id_
        , map_id_(player.GetSession()->GetMap()->GetId()) { // сохраняем map_id, чтобы при десериализации из game получить сессию для игрока по map_id
    }

    [[nodiscard]] Player Restore(Game& game) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& *map_id_;
    }

    Dog::Id id_ = Dog::Id{0u};
    Map::Id map_id_ = Map::Id{""};
};

class PlayersRepr {
public:
    PlayersRepr() = default;

    explicit PlayersRepr(const Players& players) {
        for (const auto& player : players.GetPlayers()) {
            players_.emplace_back(*player);
        }
    }

    void Restore(Game& game, Players& players) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
    }

    std::vector<PlayerRepr> players_;
};

class PlayerTokensRepr {
public:
    PlayerTokensRepr() = default;

    explicit PlayerTokensRepr(const PlayerTokens& players_and_tokens) {
        for (auto& [token, player] : players_and_tokens.GetPlayerTokens()) {
            players_and_tokens_.emplace(*player->GetDogId(), *token);
        }
    }

    void Restore(const Players& players, PlayerTokens& players_and_tokens) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & players_and_tokens_;
    }

    std::string FindTokenByDogId(uint32_t id) const;

    std::unordered_map<uint32_t, std::string> players_and_tokens_; // *DogId - Token
};

class ApplicationRepr {
public:
    ApplicationRepr() = default;
    explicit ApplicationRepr(const Application& app)
        : game_repr_(GameRepr(app.GetGame()))
        , players_repr_(PlayersRepr(app.GetPlayers()))
        , player_tokens_repr_(PlayerTokensRepr(app.GetPlayerTokens())) {
        }

    void Restore(Application& app) const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
        ar & game_repr_;
        ar & players_repr_;
        ar & player_tokens_repr_;
    }

    GameRepr game_repr_;
    PlayersRepr players_repr_;
    PlayerTokensRepr player_tokens_repr_;
};

}  // namespace serialization
