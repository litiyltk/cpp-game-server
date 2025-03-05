#include "model.h"

#include <stdexcept>


namespace model {
using namespace std::literals;

std::string_view DirectionToString(Direction direction) {
    switch (direction) {
        case Direction::NORTH: return "U"sv;
        case Direction::SOUTH: return "D"sv;
        case Direction::WEST: return "L"sv;
        case Direction::EAST: return "R"sv;
        default: throw std::invalid_argument("Unknown direction");
    }
}

Direction StringToDirection(std::string_view str) {
    if (str == "U"sv) {
        return Direction::NORTH;
    } else if (str == "D"sv) {
        return Direction::SOUTH;
    } else if (str == "L"sv) {
        return Direction::WEST;
    } else if (str == "R"sv) {
        return Direction::EAST;
    } else {
        throw std::invalid_argument("Unknown direction string");
    }
}

bool IsValidDirection(std::string_view str) {
    if (str.empty()) {
        return true;
    }
    
    if (str.size() != 1) {
        return false;
    }

    char c = str[0];
    return c == 'U' || c == 'L' || c == 'R' || c == 'D';
}

bool IsRectanglesIntersect(geom::Point2D corner1A, geom::Point2D corner2A, geom::Point2D corner1B, geom::Point2D corner2B) {
    double x1_min = corner1A.x;
    double y1_min = corner1A.y;
    double x1_max = corner2A.x;
    double y1_max = corner2A.y;

    double x2_min = corner1B.x;
    double y2_min = corner1B.y;
    double x2_max = corner2B.x;
    double y2_max = corner2B.y;

    return (x1_min <= x2_max && x1_max >= x2_min && y1_min <= y2_max && y1_max >= y2_min);
}

// методы класса Road

bool Road::IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool Road::IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point Road::GetStart() const noexcept {
        return start_;
    }

    Point Road::GetEnd() const noexcept {
        return end_;
    }

    geom::Point2D Road::GetRandomPosition() const noexcept {
        geom::Point2D pos;
        if (IsHorizontal()) {
            pos.y = static_cast<double>(start_.y); 
            pos.x = GetRandomNumber(static_cast<double>(start_.x), static_cast<double>(end_.x)); 
        } else {
            pos.x = static_cast<double>(start_.x); 
            pos.y = GetRandomNumber(static_cast<double>(start_.y), static_cast<double>(end_.y)); 
        }
        return pos;
    }

    bool Road::IsPositionOnRoad(geom::Point2D pos) const noexcept {
        double x1, x2, y1, y2; // левый нижний x1, y1, правый верхний x2, y2 для декартовой плоскости
        x1 = start_.x - half_width_;
        x2 = end_.x + half_width_;
        y1 = start_.y - half_width_;
        y2 = end_.y + half_width_;
        return pos.x >= x1 && pos.x <= x2 && pos.y >= y1 && pos.y <= y2;
    }

    // позиция на границе дороги по направлению движения, можно указать сдвиг от границы: + наружу, - на дороге
    geom::Point2D Road::GetBoundaryPositionWithOffset(geom::Point2D pos, Direction direction, double offset) const noexcept {
        geom::Point2D new_pos = { pos.x, pos.y };
        switch (direction) {
            case Direction::NORTH:
                new_pos.y = start_.y - half_width_ - offset;
                break;
            case Direction::SOUTH:
                new_pos.y = end_.y + half_width_ + offset;
                break;
            case Direction::WEST:
                new_pos.x = start_.x - half_width_ - offset;
                break;
            case Direction::EAST:
                new_pos.x = end_.x + half_width_ + offset;
                break;
        }
        return new_pos;
    }

    // левый нижний угол дороги на декартовой плоскости
    geom::Point2D Road::GetMinVertexPosition() const noexcept {
        return { start_.x - half_width_, start_.y - half_width_ };
    }

    // правый верхний угол дороги на декартовой плоскости
    geom::Point2D Road::GetMaxVertexPosition() const noexcept {
        return { end_.x + half_width_, end_.y + half_width_ };
    }

    // проверяет пересечение с другой дорогой
    bool Road::IsIntersect(const RoadPtr other) const noexcept {
        geom::Point2D corner1A = other->GetMinVertexPosition();
        geom::Point2D corner2A = other->GetMaxVertexPosition();
        geom::Point2D corner1B = GetMinVertexPosition();
        geom::Point2D corner2B = GetMaxVertexPosition();
        return IsRectanglesIntersect(corner1A, corner2A, corner1B, corner2B);
    }

    // все целочисленные координаты на отрезке дороги (x для горизонтальной, y для вертикальной)
    std::unordered_set<Coord> Road::GetCoords() const {
        std::unordered_set<Coord> coords;
        if (IsHorizontal()) {
            for (int x = start_.x; x <= end_.x; ++x) {
                coords.insert(x);
            }
        } else {
            for (int y = start_.y; y <= end_.y; ++y) {
                coords.insert(y);
            }
        }
        return coords;
    }

// методы класса Building

    const Rectangle& Building::GetBounds() const noexcept {
        return bounds_;
    }

// методы класса Office

    const Office::Id& Office::GetId() const noexcept {
        return id_;
    }

    Point Office::GetPosition() const noexcept {
        return position_;
    }

    Offset Office::GetOffset() const noexcept {
        return offset_;
    }

// методы класса Map

    const Map::Id& Map::GetId() const noexcept {
        return id_;
    }

    const std::string& Map::GetName() const noexcept {
        return name_;
    }

    const Map::Buildings& Map::GetBuildings() const noexcept {
        return buildings_;
    }

    const RoadPtrs& Map::GetRoads() const noexcept {
        return road_ptrs_;
    }

    const Map::Offices& Map::GetOffices() const noexcept {
        return offices_;
    }

    const Map::LootTypes& Map::GetLootTypes() const noexcept {
        return loot_types_;
    }

    size_t Map::GetLootTypesCount() const noexcept {
        return loot_types_.size();
    }

    const RoadPtr Map::GetRandomRoad() const noexcept {
        if (road_ptrs_.empty()) {
            return nullptr;
        }

        size_t start = 0;
        size_t ind = GetRandomNumber(start, road_ptrs_.size() - 1);
        return road_ptrs_.at(ind);
    }

    std::optional<double> Map::GetSpeed() const noexcept {
        return speed_;
    }

    std::optional<size_t> Map::GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    void Map::AddRoad(const Road& road) {
        if (road.IsHorizontal()) { // для горизонтальной дороги
            const auto road_ptr_by_start = FindHorizontalRoadByPoint(road.GetStart()); // для проверки пересечения с дорогой в начале новой дороги
            const auto road_ptr_by_end = FindHorizontalRoadByPoint(road.GetEnd()); // для проверки пересечения с дорогой в конце новой дороги

            if (road_ptr_by_start && road_ptr_by_end) { // 1. пересечение с двух концов
                Road new_road = Road(model::Road::HORIZONTAL, road_ptr_by_start->GetStart(), road_ptr_by_end->GetEnd().x); // новая дорога от начала road_ptr_by_start до конца road_ptr_by_end
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_y = new_road.GetStart().y;
                for (const auto coord : new_road.GetCoords()) {
                    horizontal_road_by_point_[Point{coord, coord_y}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_start); // удалить road_ptr_by_start
                RemoveRoad(road_ptr_by_end); // удалить road_ptr_by_end

            } else if (road_ptr_by_start) { // 2. есть пересечение только со start
                Road new_road = Road(model::Road::HORIZONTAL, road_ptr_by_start->GetStart(), road.GetEnd().x); // новая дорога от начала road_ptr_by_start до конца road
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_y = new_road.GetStart().y;
                for (const auto coord : new_road.GetCoords()) {
                    horizontal_road_by_point_[Point{coord, coord_y}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_start); // удалить road_ptr_by_start

            } else if (road_ptr_by_end) { // 3. есть пересечение только со end
                Road new_road = Road(model::Road::HORIZONTAL, road.GetStart(), road_ptr_by_end->GetEnd().x); // новая дорога от начала road до конца road_ptr_by_end
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_y = new_road.GetStart().y;
                for (const auto coord : new_road.GetCoords()) {
                    horizontal_road_by_point_[Point{coord, coord_y}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_end); // удалить road_ptr_by_end

            } else { // 4. если нет пересечения ни на start, ни на end - просто создаём указатель на новую дорогу
                RoadPtr road_ptr = std::make_shared<Road>(road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_y = road.GetStart().y;
                for (const auto coord : road.GetCoords()) {
                    horizontal_road_by_point_[Point{coord, coord_y}] = road_ptrs_.back();
                }
            }

        } else { // аналогично для вертикальной дороги
            const auto road_ptr_by_start = FindVerticalRoadByPoint(road.GetStart()); // для проверки пересечения с дорогой в начале новой дороги
            const auto road_ptr_by_end = FindVerticalRoadByPoint(road.GetEnd()); // для проверки пересечения с дорогой в конце новой дороги
            if (road_ptr_by_start && road_ptr_by_end) { // 1. пересечение с двух концов
                Road new_road = Road(model::Road::VERTICAL, road_ptr_by_start->GetStart(), road_ptr_by_end->GetEnd().y); // новая дорога от начала road_ptr_by_start до конца road_ptr_by_end
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_x = new_road.GetStart().x;
                for (const auto coord : new_road.GetCoords()) {
                    vertical_road_by_point_[Point{coord_x, coord}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_start); // удалить road_ptr_by_start
                RemoveRoad(road_ptr_by_end); // удалить road_ptr_by_end

            } else if (road_ptr_by_start) { // 2. есть пересечение только со start
                Road new_road = Road(model::Road::VERTICAL, road_ptr_by_start->GetStart(), road.GetEnd().y); // новая дорога от начала road_ptr_by_start до конца road
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_x = new_road.GetStart().x;
                for (const auto coord : new_road.GetCoords()) {
                    vertical_road_by_point_[Point{coord_x, coord}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_start); // удалить road_ptr_by_start

            } else if (road_ptr_by_end) { // 3. есть пересечение только со end
                Road new_road = Road(model::Road::VERTICAL, road.GetStart(), road_ptr_by_end->GetEnd().y); // новая дорога от начала road до конца road_ptr_by_end
                RoadPtr road_ptr = std::make_shared<Road>(new_road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_x = new_road.GetStart().x;
                for (const auto coord : new_road.GetCoords()) {
                    vertical_road_by_point_[Point{coord_x, coord}] = road_ptrs_.back();
                }
                RemoveRoad(road_ptr_by_end); // удалить road_ptr_by_end

            } else { // 4. если нет пересечения ни на start, ни на end - просто создаём указатель на новую дорогу
                RoadPtr road_ptr = std::make_shared<Road>(road);
                road_ptrs_.push_back(road_ptr);
                const auto coord_x = road.GetStart().x;
                for (const auto coord : road.GetCoords()) {
                    vertical_road_by_point_[Point{coord_x, coord}] = road_ptrs_.back();
                }
            }
        }
    }

    void Map::AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void Map::AddLootType(const LootType& loot_type) {
        loot_types_.emplace_back(loot_type);
    }

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse"s);
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } catch (const std::exception& ex) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    void Map::SetDogSpeed(double speed) {
        speed_ = speed;
    }

    void Map::SetDogBagCapacity(size_t bag_capacity) {
        bag_capacity_ = bag_capacity;
    }

    const RoadPtr Map::FindRoadByPositionAndDirection(geom::Point2D pos, Direction dir, bool by_direction) const noexcept {
        Point point = Point{static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y))};

        if (by_direction) { // дороги по направлению движения
            if (dir == Direction::EAST || dir == Direction::WEST) {
                return FindHorizontalRoadByPoint(point);
            }
            return FindVerticalRoadByPoint(point);
        } else { // дороги поперек направления движения
            if (dir == Direction::EAST || dir == Direction::WEST) {
                return FindVerticalRoadByPoint(point);
            }
            return FindHorizontalRoadByPoint(point);
        }
    }

    const RoadPtr Map::FindHorizontalRoadByPoint(const Point& point) const noexcept {
        return horizontal_road_by_point_.contains(point) ? horizontal_road_by_point_.at(point) : nullptr;
    }

    const RoadPtr Map::FindVerticalRoadByPoint(const Point& point) const noexcept {
        return vertical_road_by_point_.contains(point) ? vertical_road_by_point_.at(point) : nullptr;
    }

    void Map::RemoveRoad(const RoadPtr road) {
    if (!road) {
        return;
    }

    auto it = std::find_if(road_ptrs_.begin(), road_ptrs_.end(), [road](const RoadPtr& temp_road) {
        return temp_road == road;
    });

    if (it != road_ptrs_.end()) {
        road_ptrs_.erase(it);
    }
}

// методы класса Dog

    const Dog::Id& Dog::GetId() const noexcept {
        return id_;
    }

    const std::string& Dog::GetName() const noexcept {
        return name_;
    }

    const geom::Point2D Dog::GetPosition() const noexcept {
        return pos_;
    }

    const geom::Point2D Dog::GetPrevPosition() const noexcept {
        return prev_pos_;
    }

    const geom::Vec2D Dog::GetSpeed() const noexcept {
        return speed_;
    }

    double Dog::GetDefaultSpeed() const noexcept {
        return default_speed_;
    }

    Direction Dog::GetDirection() const noexcept {
        return dir_;
    }

    size_t Dog::GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    bool Dog::IsBagFull() const noexcept {
        return bag_.size() >= bag_capacity_;
    }

    const LootPtrs& Dog::GetBag() const noexcept {
        return bag_;
    }

    void Dog::AddLootIntoBag(LootPtr loot) {
        bag_.push_back(std::move(loot));
    }

    void Dog::IncreaseScore(size_t score) noexcept {
        score_ += score;
    }

    void Dog::UpdateScore(const Map::LootTypes& loot_types) {
        for (const auto& loot_in_bag : bag_) {
            size_t score_for_loot = std::get<int64_t>(loot_types.at(loot_in_bag->type).properties.at("value"));
            IncreaseScore(score_for_loot);
        }
        bag_.clear();
    }

    size_t Dog::GetScore() const noexcept {
        return score_;
    }

    void Dog::SetDirectionSpeed(std::string_view str) {
        if (str.empty()) { // остановка
            speed_ = { 0.0, 0.0 };
        } else {
            dir_ = StringToDirection(str);
            switch (dir_) {
                case Direction::WEST:
                    speed_.x = -default_speed_;
                    speed_.y = 0.0;
                    break;
                case Direction::EAST:
                    speed_.x = default_speed_;
                    speed_.y = 0.0;
                    break;
                case Direction::NORTH:
                    speed_.x = 0.0;
                    speed_.y = -default_speed_;
                    break;
                case Direction::SOUTH:
                    speed_.x = 0.0;
                    speed_.y = default_speed_;
                    break;
                default:
                    throw std::invalid_argument("Unknown direction");
            }
        }
    }

    void Dog::SetPosition(geom::Point2D pos) noexcept {
        prev_pos_ = pos_; // позиция до начала хода
        pos_ = pos; // позиция после хода
    }

    void Dog::SetSpeed(geom::Vec2D speed) noexcept {
        speed_ = speed;
    }

    void Dog::IncreasePlayTime(const int time_delta) noexcept {
        play_time_ += static_cast<uint32_t>(time_delta);
    }

    void Dog::SetPlayTime(const int time_delta) noexcept {
        play_time_ = static_cast<uint32_t>(time_delta);
    }

    uint32_t Dog::GetPlayTime() const noexcept {
        return play_time_;
    }

    void Dog::ResetInactivityTime() noexcept {
        inactivity_time_ = 0;
    }

    void Dog::IncreaseInactivityTime(const int time_delta) noexcept {
        inactivity_time_ += static_cast<uint32_t>(time_delta);
    }

    void Dog::SetInactivityTime(const int time_delta) noexcept {
        inactivity_time_ = static_cast<uint32_t>(time_delta);
    }

    uint32_t Dog::GetInactivityTime() const noexcept {
        return inactivity_time_;
    }

    bool Dog::IsMoving() const noexcept {
        return is_moving_;
    }

    void Dog::Stopped() noexcept {
        is_moving_ = false;
    }

    void Dog::Moving() noexcept {
        is_moving_ = true;
    }

// методы класса GameSession

    void GameSession::AddDog(DogPtr dog) {
        dogs_.push_back(std::move(dog));
    }

    void GameSession::AddLoot(LootPtr loot) {
        loots_.push_back(std::move(loot));
    }

    const Map* GameSession::GetMap() const noexcept {
        return map_;
    }

    DogPtr GameSession::GetDog(Dog::Id id) noexcept {
        auto it = std::find_if(dogs_.begin(), dogs_.end(), [&id](DogPtr& dog) {
            return dog->GetId() == id;
        });

        return (it != dogs_.end()) ? *it : nullptr;
    }

    const DogPtrs& GameSession::GetDogs() const noexcept {
        return dogs_;
    }

    const LootPtrs& GameSession::GetLoots() const noexcept {
        return loots_;
    }

    size_t GameSession::GetDogsCount() const noexcept {
        return dogs_.size();
    }

    size_t GameSession::GetLootsCount() const noexcept {
        return loots_.size();
    }

    LootPtr GameSession::TakeLoot(Loot::Id id) {
        auto it = std::find_if(loots_.begin(), loots_.end(), [&id](const LootPtr& loot) {
            return loot->id == id;
        });

        if (it != loots_.end()) { // забираем предмет
            LootPtr loot = std::move(*it); // передаем владение
            *it = nullptr;
            loots_.erase(it); // удаляем
            return loot; // возвращаем владение
        }

        return nullptr;
    }

    loot_gen::LootGenerator* GameSession::GetLootGenerator() noexcept {
        return &loot_generator_;
    }

    void GameSession::RemoveDogById(Dog::Id id) {
        auto it = std::find_if(dogs_.begin(), dogs_.end(), [&id](DogPtr& dog) {
            return dog->GetId() == id;
        });

        if (it != dogs_.end()) {
            dogs_.erase(it);
        }
    }

// методы класса Game

    void Game::AddMap(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } else {
            try {
                maps_.emplace_back(std::move(map));
            } catch (const std::exception& ex) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::SetDefaultSpeed(double speed) noexcept {
        default_speed_ = speed;
    }

    void Game::SetDefaultBagCapacity(size_t capacity) noexcept {
        default_bag_capacity_ = capacity;
    }

    void Game::SetRetirementTime(double time_in_sec) noexcept {
        dog_retirement_time_in_sec_ = time_in_sec;
    }

    void Game::SetLootConfig(double period, double probability) noexcept {
        loot_period_ = period;
        loot_probability_ = probability;
    }

    double Game::GetDefaultSpeed() const noexcept {
        return default_speed_;
    }

    size_t Game::GetDefaultBagCapacity() const noexcept {
        return default_bag_capacity_;
    }

    double Game::GetRetirementTime() const noexcept {
        return dog_retirement_time_in_sec_;
    }

    double Game::GetLootPeriod() const noexcept {
        return loot_period_;
    }

    double Game::GetLootProbability() const noexcept {
        return loot_probability_;
    }

    const Game::Maps& Game::GetMaps() const noexcept {
        return maps_;
    }

    const Map* Game::FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }


    void Game::AddSession(GameSessionPtr gamession) {
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_session_.emplace(gamession->GetMap()->GetId(), index); !inserted) {
            throw std::invalid_argument("GameSession with id "s + *gamession->GetMap()->GetId() + " already exists"s);
        } else {
            try {
                sessions_.emplace_back(std::move(gamession));
            } catch (const std::exception& ex) {
                map_id_to_session_.erase(it);
                throw;
            }
        } 
    }

    GameSessionPtr Game::GetSession(const Map::Id& id) noexcept {
        if (auto it = map_id_to_session_.find(id); it != map_id_to_session_.end()) {
            return sessions_.at(it->second);
        }
        return nullptr;
    }

    size_t Game::GetSessionsCount() const noexcept {
        return sessions_.size();
    }

    bool Game::HasSession(const Map::Id& id) const noexcept {
        return map_id_to_session_.find(id) != map_id_to_session_.end();
    }

    const GameSessionPtrs& Game::GetSessions() const noexcept {
        return sessions_;
    }

    uint32_t Game::GetTotalLootsCount() const noexcept { // для нового уникального Loot::Id
        return total_loot_count_;
    }

    void Game::SetTotalLootsCount(uint32_t loot_count) noexcept { // для восстановления состояния игры
        total_loot_count_ = loot_count;
    }

    void Game::IncreaseTotalLootsCount() noexcept { // для добавления нового предмета
        ++total_loot_count_;
    }

    uint32_t Game::GetTotalDogsCount() const noexcept { // для нового уникального Dog::Id
        return total_dog_count_;
    }

    void Game::SetTotalDogsCount(uint32_t dog_count) noexcept { // для восстановления состояния игры
        total_dog_count_ = dog_count;
    }

    void Game::IncreaseTotalDogsCount() noexcept { // для добавления новой собаки игрока
        ++total_dog_count_;
    }

}  // namespace model
