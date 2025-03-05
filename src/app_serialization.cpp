#include "app_serialization.h"


namespace serialization {

// методы класса PlayerRepr

    [[nodiscard]] Player PlayerRepr::Restore(Game& game) const {
        Player player = Player(id_, game.GetSession(map_id_));
        return player;
    }

// методы класса PlayersRepr


    void PlayersRepr::Restore(Game& game, Players& players) const {
        for (const auto& player_repr : players_) {
            Player player = player_repr.Restore(game);
            players.RestorePlayer(player);
        }
    }

// методы класса PlayerTokensRepr

    void PlayerTokensRepr::Restore(const Players& players, PlayerTokens& players_and_tokens) const {
        for (auto& player : players.GetPlayers()) {
            const Token token = Token(FindTokenByDogId(*player->GetDogId()));
            players_and_tokens.RestoreTokenAndPlayer(token, player);
        }
    }

    std::string PlayerTokensRepr::FindTokenByDogId(uint32_t id) const {
        auto it = players_and_tokens_.find(id);
        if (it != players_and_tokens_.end()) {
            return it->second;
        }
        throw std::runtime_error("Token not found for DogId: " + std::to_string(id));
    }

// методы класса ApplicationRepr

    void ApplicationRepr::Restore(Application& app) const {
        game_repr_.Restore(app.GetGame());
        players_repr_.Restore(app.GetGame(), app.GetPlayers());
        player_tokens_repr_.Restore(app.GetPlayers(), app.GetPlayerTokens());
    }

}  // namespace serialization
