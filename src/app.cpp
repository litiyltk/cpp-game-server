#include "app.h"

namespace app {

using namespace model;

bool IsValidTokenString(const std::string& str) {
    const std::regex hex_pattern("^[0-9a-fA-F]{32}$");
    return std::regex_match(str, hex_pattern);
}

geom::Point2D CalcNewPosition(const geom::Point2D& pos, const geom::Vec2D& speed, int time_delta) {
    const double x = pos.x + static_cast<double>(speed.x) * static_cast<double>(time_delta) / MILLISECONDS_PER_SECOND;
    const double y = pos.y + static_cast<double>(speed.y) * static_cast<double>(time_delta) / MILLISECONDS_PER_SECOND;
    geom::Point2D new_pos = {x, y};
    return new_pos;
}

// методы класса Player

    Dog::Id Player::GetDogId() const noexcept {
        return dog_id_;
    }

    GameSessionPtr Player::GetSession() const noexcept {
        return session_;
    }

// методы класса Players

    // при добавлении игрока возвращаем shared_ptr на него
    PlayerPtr Players::Add(DogPtr dog, GameSessionPtr session) {
        session->AddDog(dog);
        Player player(dog->GetId(), session); // Создаём нового игрока
        players_.push_back(std::move(std::make_shared<Player>(player)));
        return players_.back();
    }

    PlayerPtr Players::FindByDogId(Dog::Id dog_id) noexcept {
        for (auto& player : players_) {
            if (*player->GetDogId() == *dog_id) {
                return player;
            }
        }
        return nullptr;
    }

    std::size_t Players::CountPlayersAtMap(const Map::Id map_id) const noexcept {
        return std::count_if(players_.begin(), players_.end(), [&map_id](const PlayerPtr& player) {
            return *player->GetSession()->GetMap()->GetId() == *map_id;
        });
    }

    std::size_t Players::CountPlayers() const noexcept {
        return players_.size();
    }

    const PlayerPtrs& Players::GetPlayers() const noexcept {
        return players_;
    }

    // для записи всех игрока при восстановлении игры
    void Players::RestorePlayer(const Player& player) {
        players_.push_back(std::move(std::make_shared<Player>(player)));
    }

    // для удаления игрока по Dog::Id при завершении игры
    void Players::RemoveByDogId(Dog::Id dog_id) {
        auto it = std::find_if(players_.begin(), players_.end(), [&dog_id](const PlayerPtr& player) {
            return *player->GetDogId() == *dog_id;
        });

        if (it != players_.end()) {
            players_.erase(it);
        }
    }

// методы класс PlayerTokens

    PlayerPtr PlayerTokens::FindPlayerByToken(const Token& token) const noexcept {
        if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const Token PlayerTokens::AddPlayer(PlayerPtr player) {
        const auto token = GenerateToken();
        token_to_player_.emplace(token, std::move(player));
        return token;
    }

    const PlayerTokens::player_tokens& PlayerTokens::GetPlayerTokens() const noexcept {
        return token_to_player_;
    }

    // для записи всех игроков при восстановлении игры
    void PlayerTokens::RestoreTokenAndPlayer(const Token token, PlayerPtr player) {
        token_to_player_.emplace(token, player);
    }

    // для удаления токена и игрока при завершении игры
    void PlayerTokens::RemovePlayerTokenByDogId(Dog::Id dog_id) {
        for (auto it = token_to_player_.begin(); it != token_to_player_.end();) {
            if (*it->second->GetDogId() == *dog_id) {
                it = token_to_player_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::mt19937_64 PlayerTokens::init_generator() {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return std::mt19937_64(dist(random_device_));
    }

    const Token PlayerTokens::GenerateToken() {
        auto token1 = generator1_();
        auto token2 = generator2_();

        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << token1
           << std::setw(16) << std::setfill('0') << token2;
        return Token(ss.str());
    }

// методы класса Application

    const Game::Maps& Application::GetMaps() const noexcept {
        return game_.GetMaps();
    }

    const Map* Application::FindMap(const Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

    PlayerPtr Application::FindPlayerByToken(const Token& token) const noexcept {
        return player_tokens_.FindPlayerByToken(token);
    }

    const DogPtrs& Application::GetDogs(PlayerPtr player) const noexcept {
        return player->GetSession()->GetDogs();
    }

    const LootPtrs& Application::GetLoots(PlayerPtr player) const noexcept {
        return player->GetSession()->GetLoots();
    }

    JoinInfo Application::JoinGame(const std::string& name, const Map::Id& map_id) {
        if (!game_.HasSession(map_id)) { // сессия привязана к карте (будем считать, что одна карта = одна сессия)
            GameSession session(game_.FindMap(map_id), game_.GetLootPeriod(), game_.GetLootProbability());
            // при необходимости создать сессию для карты (будем считать, что число карт >= число сессий)
            game_.AddSession(std::move(std::make_shared<GameSession>(session)));
        }

        Dog::Id dog_id{static_cast<uint32_t>(game_.GetTotalDogsCount())}; // определить идентификатор собаки
        game_.IncreaseTotalDogsCount(); // +1 для следующего id

        geom::Point2D pos;
        if (randomize_spawn_points_) { // определить случайные стартовые координаты
            pos = game_.FindMap(map_id)->GetRandomRoad()->GetRandomPosition(); 
        } else { // !!! для тестов: после входа в игру пёс игрока появлялся в начальной точке первой дороги карты
            Point point = game_.FindMap(map_id)->GetRoads().at(0)->GetStart();
            pos = {static_cast<double>(point.x), static_cast<double>(point.y)};
        }

        double default_speed; // определить скорость
        if (const auto default_speed_opt = game_.FindMap(map_id)->GetSpeed()) {
            default_speed = default_speed_opt.value();
        } else {
            default_speed = game_.GetDefaultSpeed();
        }

        size_t bag_capacity; // определить вместимость рюкзака
        if (const auto bag_capacity_opt = game_.FindMap(map_id)->GetBagCapacity()) {
            bag_capacity = bag_capacity_opt.value();
        } else {
            bag_capacity = game_.GetDefaultBagCapacity();
        }

        Dog dog(dog_id, name, pos, default_speed, bag_capacity); // создать собаку
        return JoinInfo{ player_tokens_.AddPlayer(players_.Add(std::move(std::make_shared<Dog>(dog)), game_.GetSession(map_id))), *dog_id };
        
    }

    // Добавляем обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    [[nodiscard]] sig::connection Application::DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }

    void Application::Tick(milliseconds delta) {
        MoveDogs(delta.count()); // 1. пересчёт позиций собак на карте за время шага Tick
        UpdateLoots(delta.count()); // 2. обновление количества предметов на карте
        HandleCollisions(); // 3. обработка столкновений и удаление предметов
        UpdateDogsTimesAndRemove(delta.count()); // 4. обновление времени игроков и удаление игроков превысивших время бездействия
        tick_signal_(delta); // Уведомляем подписчиков сигнала tick - для сохранения состояния игры
    }

    bool Application::HasTickPeriod() const noexcept {
        return tick_period_ != 0;
    }

    int Application::GetTickPeriod() const noexcept {
        return tick_period_;
    }

    // методы для сохранения состояния игры (применяются в app_serialization.h)

    const Game& Application::GetGame() const noexcept { 
        return game_;
    }

    Game& Application::GetGame() noexcept { 
        return game_;
    }

    const PlayerTokens& Application::GetPlayerTokens() const noexcept { 
        return player_tokens_;
    }

    PlayerTokens& Application::GetPlayerTokens() noexcept { 
        return player_tokens_;
    }

    const Players& Application::GetPlayers() const noexcept { 
        return players_;
    }

    Players& Application::GetPlayers() noexcept { 
        return players_;
    }

    // методы для взаимодействия с БД

    void Application::SetConnectionPool(postgres::ConnectionPoolPtr pool) {
        pool_ = pool;
    }

    postgres::PlayersRecords Application::GetRecords(int start, int max_items) {
        return postgres::DataBase::GetRecords(pool_, start, max_items);
    }

    void Application::MoveDogs(int time_delta) {
        for (auto& player : players_.GetPlayers()) { // для каждого пса игрока
            DogPtr dog = player->GetSession()->GetDog(player->GetDogId());

            const geom::Point2D pos = dog->GetPosition(); // текущая позиция на дороге
            const geom::Vec2D speed = dog->GetSpeed(); 
            const Direction dir = dog->GetDirection();
            const geom::Point2D new_pos = CalcNewPosition(pos, speed, time_delta); // новая позиция на дороге

            const Map* map = player->GetSession()->GetMap();

            // вдоль дороги
            const RoadPtr road = map->FindRoadByPositionAndDirection(pos, dir);
            const RoadPtr new_road = map->FindRoadByPositionAndDirection(new_pos, dir);

            // поперёк дороги
            const RoadPtr across_road = map->FindRoadByPositionAndDirection(pos, dir, false);
            const RoadPtr across_new_road = map->FindRoadByPositionAndDirection(new_pos, dir, false);

            // движение вдоль дороги
            if (road && new_road) { // обе точки на одной дороге - движение в конечную точку
                dog->SetPosition(new_pos);
            } else if (road && !new_road) { // начальная точка на дороге, конечная вне дороги - движение вдоль дороги на край
                dog->SetPosition(road->GetBoundaryPositionWithOffset(pos, dir));
                dog->SetSpeed({0.0, 0.0});

            // движение поперёк дороги
            } else if (across_road && across_new_road) { // обе точки на одной дороге - движение в конечную точку
                dog->SetPosition(new_pos);
            } else { // обе точки вне дороги - движение на край поперёк дороги - есть еще один случай, когда пёс не оказывается сразу на границе дороги - проверить по другим дорогам
                dog->SetPosition(across_road->GetBoundaryPositionWithOffset(pos, dir));
                dog->SetSpeed({0.0, 0.0});
            }
        }
    }

    void Application::UpdateLoots(int time_delta) {
        for (auto& game_session: game_.GetSessions()) { // проходим все сессии
            const Map* map = game_session->GetMap();

            size_t looter_count = game_session->GetDogsCount(); // игроки
            size_t loot_count = game_session->GetLootsCount(); // предметы
            auto loot_generator = game_session->GetLootGenerator(); // генератор предметов

            unsigned new_loot_count = loot_generator->Generate( // проверяем, сколько еще предметов нужно добавить
                milliseconds(time_delta), loot_count, looter_count);

            for (unsigned i = 0; i < new_loot_count; ++i) { // генерим новые предметы
                Loot loot;
                loot.id = Loot::Id{game_.GetTotalLootsCount()}; // id предмета
                game_.IncreaseTotalLootsCount(); // +1 для следующего id
                loot.type = GetRandomNumber(size_t(0), map->GetLootTypesCount() - 1);
                loot.pos = map->GetRandomRoad()->GetRandomPosition();
                game_session->AddLoot(std::move(std::make_shared<Loot>(loot)));
            }
        }
    }

    void Application::HandleCollisions() {
        using namespace collision_detector;
        for (auto& game_session: game_.GetSessions()) { // проходим все сессии
            const DogPtrs dogs = game_session->GetDogs(); // получаем вектор собирателей
            std::vector<Gatherer> gatherers;
            gatherers.reserve(dogs.size());
            for (size_t i = 0; i < dogs.size(); ++i) { // индексы собирателей совпадают с индексами Dogs в сессии на карте
                geom::Point2D start_pos = dogs.at(i)->GetPrevPosition(); // позиция до Application::MoveDogs - до начала хода
                geom::Point2D end_pos = dogs.at(i)->GetPosition(); 

                gatherers.emplace_back(Gatherer{
                    { start_pos.x, start_pos.y },
                    { end_pos.x, end_pos.y },
                    DOG_HALF_WIDTH
                });
            }

            const LootPtrs& loots = game_session->GetLoots(); // получаем ссылку вектор предметов
            const Map::Offices offices = game_session->GetMap()->GetOffices(); // получаем вектор офисов
            std::vector<Item> items;
            items.reserve(loots.size() + offices.size()); // вектор коллизий с предметами и офисами

            // Добавляем предметы (индексация items предметов совпадают с loots)
            for (size_t i = 0; i < loots.size(); ++i) {
                geom::Point2D pos = loots.at(i)->pos;
                items.push_back({ { pos.x, pos.y }, LOOT_HALF_WIDTH });
            }

            // Добавляем офисы (индексация офисов сдвинута на loots.size())
            for (size_t i = 0; i < offices.size(); ++i) {
                Point point = offices.at(i).GetPosition();
                items.push_back({ { static_cast<double>(point.x), static_cast<double>(point.y) }, OFFICE_HALF_WIDTH });
            }

            const auto events = FindGatherEvents(ModelItemGathererProvider(items, gatherers)); // получаем вектор событий для каждой сессии

            std::unordered_set<size_t> collected_loot_ids; // сохраняем индексы собранных предметов, чтобы удалить их из loots в сессии на карте
            for (const auto& event : events) {
                size_t dog_index = event.gatherer_id; // индексы gatherers и dogs совпадают
                size_t loot_or_office_index = event.item_id; // индексы items совпадают c loots, далее идут индексы offices

                const Dog::Id dog_id = game_session->GetDogs().at(dog_index)->GetId(); // получаем ID собаки
                DogPtr dog = game_session->GetDog(dog_id); // и по найденному ID получаем указатель

                if (loot_or_office_index >= loots.size()) { // офис - сбросить лут из рюкзака и получить награду за каждый тип предмета
                    const Map::LootTypes loot_types = game_session->GetMap()->GetLootTypes(); // получаем все типы предметов для карты в сессии
                    dog->UpdateScore(loot_types);

                } else { // предмет - проверить, был ли собран этот предмет ранее другим собирателем

                    if (collected_loot_ids.contains(loot_or_office_index)) {
                        continue; // игнорируем, переходим к следующему событию
                    }

                    if (!dog->IsBagFull()) {
                        Loot::Id loot_id = loots.at(loot_or_office_index)->id;
                        LootPtr loot = game_session->TakeLoot(loot_id); // забираем нужный предмет по Loot::ID из loots_
                        if (loot) {
                            dog->AddLootIntoBag(std::move(loot));
                            collected_loot_ids.insert(loot_or_office_index);
                        }
                    }
                }
            }
        }
    }

    void Application::LeaveGame(Dog::Id dog_id) {
        players_.FindByDogId(dog_id)->GetSession()->RemoveDogById(dog_id); // удаляем из Game->GameSession
        players_.RemoveByDogId(dog_id); // удаляем из Players
        player_tokens_.RemovePlayerTokenByDogId(dog_id); // удаляем из PlayerTokens
    }

    void Application::UpdateDogsTimesAndRemove(const int time_delta) {
        std::unordered_set<uint32_t> dog_ids_to_remove; // собираем id собак, превысивших время ожидания, для удаления
        for (auto& player : players_.GetPlayers()) { // для каждого пса игрока
            DogPtr dog = player->GetSession()->GetDog(player->GetDogId());

            // обновляем время бездействия
            if (dog->IsMoving()) { // собака переместилась на текущем шаге времени Tick
                dog->ResetInactivityTime();
                dog->IncreasePlayTime(time_delta); // обновляем время в игре, только когда собака движется
            } else {
                dog->IncreaseInactivityTime(time_delta);
            }

            dog->Stopped(); // сбрасываем метку движения на текущем тике

            // проверяем превышение времени бездействия
            if (dog->GetInactivityTime() >= game_.GetRetirementTime() * MILLISECONDS_PER_SECOND) {
                dog_ids_to_remove.insert(*dog->GetId());

                // сохраняем статистику в БД
                postgres::PlayerRecord record;
                record.uuid = util::detail::UUIDToString(util::detail::NewUUID());
                record.name = dog->GetName();
                record.score = dog->GetScore();
                // Дословно: "Время, которое игрок провёл в игре, включает в себя время бездействия, прошедшее с момента последней остановки.
                record.play_time_ms = static_cast<int>(dog->GetPlayTime() + game_.GetRetirementTime() * MILLISECONDS_PER_SECOND);
                postgres::DataBase::AddRecord(pool_, record);
            }
        }

        // удаляем собак, превысивших время ожидания
        for (const auto id : dog_ids_to_remove) {
            LeaveGame(Dog::Id{id});
        }
    }

} // namespace app