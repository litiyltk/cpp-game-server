#pragma once

#include "geom.h" // для geom::Point2D
#include "collision_detector.h" // для обработки столкновений в HandleCollisions
#include "model.h" // сушности для игры Dog, Map, Loot
#include "postgres.h" // для сохранения рекордов в БД при удалении из игры
#include "tagged.h" // для Token
#include "tagged_uuid.h" // для uuid при добавлении в БД

#include <boost/signals2.hpp> // для Application::DoOnTick
#include <chrono> // для Application::Tick
#include <cstdint> // uint32_t в ::ID
#include <random> // для генератора токена
#include <regex> // для проверки токена
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map> // для токенов игроков в PlayerTokens
#include <unordered_set> // для учета предметов HandleCollisions и id собак в UpdateDogsTimesAndRemove


namespace app {

using namespace model;

namespace sig = boost::signals2;
using milliseconds = std::chrono::milliseconds;
using namespace std::literals;

class Player;
using PlayerPtr = std::shared_ptr<Player>;
using PlayerPtrs = std::vector<std::shared_ptr<Player>>;

    namespace detail {
    
    struct TokenTag {};

    }  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

struct JoinInfo {
    Token token;
    uint32_t id; 
};

bool IsValidTokenString(const std::string& str);
geom::Point2D CalcNewPosition(const geom::Point2D& pos, const geom::Vec2D& speed, int time_delta);

class Player {
public:
    explicit Player(Dog::Id dog_id, GameSessionPtr session)
        : dog_id_{dog_id}
        , session_{session} {
    }

    Dog::Id GetDogId() const noexcept;
    GameSessionPtr GetSession() const noexcept;

private:
    Dog::Id dog_id_;
    GameSessionPtr session_;
};

class Players {
public:
    PlayerPtr Add(DogPtr dog, GameSessionPtr session); // при добавлении игрока возвращаем shared_ptr на него
    PlayerPtr FindByDogId(Dog::Id dog_id) noexcept;
    std::size_t CountPlayersAtMap(const Map::Id map_id) const noexcept;
    std::size_t CountPlayers() const noexcept;
    const PlayerPtrs& GetPlayers() const noexcept;
    void RestorePlayer(const Player& player); // для записи всех игроков при восстановлении игры
    void RemoveByDogId(Dog::Id dog_id); // для удаления игрока по Dog::Id при завершении игры

private:
    PlayerPtrs players_;
};

class PlayerTokens {
public:
    using player_tokens = std::unordered_map<const Token, PlayerPtr, util::TaggedHasher<const Token>>;

    PlayerTokens()
        : generator1_(init_generator())
        , generator2_(init_generator()) {
    }

    PlayerPtr FindPlayerByToken(const Token& token) const noexcept;
    const Token AddPlayer(PlayerPtr player);
    const player_tokens& GetPlayerTokens() const noexcept;
    void RestoreTokenAndPlayer(const Token token, PlayerPtr player); // для записи всех игроков при восстановлении игры
    void RemovePlayerTokenByDogId(Dog::Id dog_id); // для удаления токена и игрока при завершении игры

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_;
    std::mt19937_64 generator2_;
    player_tokens token_to_player_;

    std::mt19937_64 init_generator();
    const Token GenerateToken();
};

class Application {
public:
    using TickSignal = sig::signal<void(milliseconds delta)>;

    explicit Application(Game& game, int tick_period, bool randomize_spawn_points)
        : game_{game}
        , tick_period_(tick_period)
        , randomize_spawn_points_(randomize_spawn_points) {
    }

    const Game::Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;
    PlayerPtr FindPlayerByToken(const Token& token) const noexcept;
    const DogPtrs& GetDogs(PlayerPtr player) const noexcept;
    const LootPtrs& GetLoots(PlayerPtr player) const noexcept;
    JoinInfo JoinGame(const std::string& name, const Map::Id& map_id);
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler);
    void Tick(milliseconds delta);
    bool HasTickPeriod() const noexcept;
    int GetTickPeriod() const noexcept;

    // методы для сохранения состояния игры (применяются в app_serialization.h)

    const Game& GetGame() const noexcept;
    Game& GetGame() noexcept;
    const PlayerTokens& GetPlayerTokens() const noexcept;
    PlayerTokens& GetPlayerTokens() noexcept;
    const Players& GetPlayers() const noexcept;
    Players& GetPlayers() noexcept;

    // методы для взаимодействия с БД

    void SetConnectionPool(postgres::ConnectionPoolPtr pool);
    postgres::PlayersRecords GetRecords(int start, int max_items);

private:
    Game& game_;
    int tick_period_;
    bool randomize_spawn_points_;
    
    PlayerTokens player_tokens_;
    Players players_;

    TickSignal tick_signal_;

    postgres::ConnectionPoolPtr pool_;

    void MoveDogs(int time_delta);
    void UpdateLoots(int time_delta);
    void HandleCollisions();
    void LeaveGame(Dog::Id dog_id);
    void UpdateDogsTimesAndRemove(int time_delta);
};

} // namespace app