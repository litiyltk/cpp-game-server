#pragma once

#include <cmath> // для round
#include <cstdint> // uint32_t
#include <iomanip>
#include <optional> // speed_, bag_capacity, Loot, 
#include <memory> // для shared_ptr
#include <random>
#include <stdexcept>  // для std::out_of_range в GetRandomRoad
#include <string>
#include <variant> // для .Properties для LootType
#include <vector>
#include <unordered_map>
#include <unordered_set> // для Road::GetPoints()

#include "geom.h" // для ::Point2D
#include "loot_generator.h" // для генератора предметов в каждой GameSession
#include "tagged.h" // для ::ID


namespace model {

constexpr double DEFAULT_DOG_SPEED = 1.0;
constexpr size_t DEFAULT_DOG_BAG_CAPACITY = 3;
constexpr double DEFAULT_DOG_RETIREMENT_TIME_IN_SEC = 60.0;

constexpr double ROAD_HALF_WIDTH = 0.8 / 2;

constexpr double DOG_HALF_WIDTH = 0.6 / 2; // 0.6 ширина собаки
constexpr double OFFICE_HALF_WIDTH = 0.5 / 2; // 0.5 ширина офиса
constexpr double LOOT_HALF_WIDTH = 0.0;

constexpr size_t SIMPLE_NUMBER = 67; // для хэшера Point::Hasher
//constexpr double EPS = 1.0e-10; // точность сравнения double

constexpr int MILLISECONDS_PER_SECOND = 1000;

using Dimension = int;
using Coord = Dimension;

using namespace std::chrono;

class Road;
using RoadPtr = std::shared_ptr<Road>;
using RoadPtrs = std::vector<RoadPtr>;

class Dog; 
using DogPtr = std::shared_ptr<Dog>;
using DogPtrs = std::vector<DogPtr>;

struct Loot;
using LootPtr = std::shared_ptr<Loot>;
using LootPtrs = std::vector<LootPtr>;

class GameSession;
using GameSessionPtr = std::shared_ptr<GameSession>;
using GameSessionPtrs = std::vector<GameSessionPtr>;

struct Point {
    Coord x, y;

    bool operator==(const Point& other) const noexcept {
        return x == other.x && y == other.y;
    }

    struct Hasher {
        size_t operator() (const Point& point) const noexcept { 
            //return hasher_(point.x) + hasher_(point.y) * SIMPLE_NUMBER * SIMPLE_NUMBER;
            //return hasher_(point.x) ^ (hasher_(point.y) << 16);
            return hasher_(point.x) ^ (hasher_(point.y) * SIMPLE_NUMBER);
        }

    private:
        std::hash<Coord> hasher_;
    };
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

std::string_view DirectionToString(Direction direction);
Direction StringToDirection(std::string_view str);
bool IsValidDirection(std::string_view str);

// нужны: левый нижний и правый верхний угол на декартовой плоскости
bool IsRectanglesIntersect(geom::Point2D corner1A, geom::Point2D corner2A, geom::Point2D corner1B, geom::Point2D corner2B);

template <typename T>
T GetRandomNumber(T start, T end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> dis(std::min(start, end), std::max(start, end));
        return dis(gen);
    } else {
        std::uniform_real_distribution<T> dis(static_cast<double>(std::min(start, end)), static_cast<double>(std::max(start, end)));
        return dis(gen);
    }
}

// Произвольные свойства - std::variant
using Properties = std::unordered_map<std::string, std::variant<std::string, double, int64_t, bool>>;

struct LootType {
    Properties properties; 
};

struct Loot {
    using Id = util::Tagged<std::uint32_t, Loot>;
    Loot::Id id{0};
    int type;
    geom::Point2D pos;

    bool operator==(const Loot& other) const {
        return id == other.id &&
               type == other.type &&
               pos == other.pos;
    }

    bool operator!=(const Loot& other) const {
        return !(*this == other);
    }
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept { // начальная точка с минимальными координатами
        Coord min_x = std::min(start.x, end_x);
        Coord max_x = std::max(start.x, end_x);
        start_ = Point{min_x, start.y};
        end_ = Point{max_x, start.y};
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept { // начальная точка с минимальными координатами
        Coord min_y = std::min(start.y, end_y);
        Coord max_y = std::max(start.y, end_y);
        start_ = Point{start.x, min_y};
        end_ = Point{start.x, max_y};
    }

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    geom::Point2D GetRandomPosition() const noexcept;
    bool IsPositionOnRoad(geom::Point2D pos) const noexcept; // для тестов - проверка добавленной дороги по координатам

    // позиция на границе дороги по направлению движения, можно указать сдвиг от границы: + наружу, - на дороге
    geom::Point2D GetBoundaryPositionWithOffset(geom::Point2D pos, Direction direction, double offset = 0.0) const noexcept;

    geom::Point2D GetMinVertexPosition() const noexcept; // левый нижний угол дороги на декартовой плоскости
    geom::Point2D GetMaxVertexPosition() const noexcept; // правый верхний угол дороги на декартовой плоскости
    bool IsIntersect(const RoadPtr other) const noexcept; // проверяет пересечение с другой дорогой

    std::unordered_set<Coord> GetCoords() const; // целочисленные координаты точки на оси дороги

private:
    Point start_;
    Point end_;
    double half_width_ = ROAD_HALF_WIDTH;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using LootTypes = std::vector<LootType>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , speed_(std::nullopt)
        , bag_capacity_(std::nullopt) {
    }

    const Id& GetId() const noexcept;

    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;

    const RoadPtrs& GetRoads() const noexcept;

    const Offices& GetOffices() const noexcept;
    const LootTypes& GetLootTypes() const noexcept;
    size_t GetLootTypesCount() const noexcept;

    const RoadPtr GetRandomRoad() const noexcept;

    std::optional<double> GetSpeed() const noexcept;
    std::optional<size_t> GetBagCapacity() const noexcept;

    void AddRoad(const Road& road);

    void AddBuilding(const Building& building);
    void AddLootType(const LootType& loot_type);
    void AddOffice(Office office);
    void SetDogSpeed(double speed);
    void SetDogBagCapacity(size_t bag_capacity);

    // поиск дороги по указанной позиции и по направлению движения по умолчанию
    const RoadPtr FindRoadByPositionAndDirection(geom::Point2D pos, Direction dir, bool by_direction = true) const noexcept;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    using PointToRoad = std::unordered_map<Point, RoadPtr, Point::Hasher>; // объединенные дороги, пересчитанные после добавления каждой

    Id id_;
    std::string name_;

    RoadPtrs road_ptrs_;
    PointToRoad horizontal_road_by_point_; // для поиска горизонтальной дороги по координате
    PointToRoad vertical_road_by_point_; // для поиска вертикальной дороги по координате

    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::optional<double> speed_; // по умолчанию использовать скорость из Game
    std::optional<size_t> bag_capacity_; // по умолчанию использовать вместимость рюкзака из Game

    LootTypes loot_types_;

    const RoadPtr FindHorizontalRoadByPoint(const Point& point) const noexcept; // поиск горизонтальной дороги по целочисленной точке
    const RoadPtr FindVerticalRoadByPoint(const Point& point) const noexcept; // поиск вертикальной дороги по целочисленной точке

    void RemoveRoad(const RoadPtr road); // для удаления дороги (повторные участки дорог при добавлении в методе Map::AddRoad)
};

class Dog {
public:
    using Id = util::Tagged<std::uint32_t, Dog>;

    Dog(Id id, std::string name, geom::Point2D pos, double default_speed, size_t bag_capacity) noexcept
        : id_{id}
        , name_{std::move(name)}
        , pos_(std::move(pos))
        , default_speed_(default_speed)
        , bag_capacity_(bag_capacity) {
    }

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const geom::Point2D GetPosition() const noexcept; // после выполнения шага Tick
    const geom::Point2D GetPrevPosition() const noexcept; // до выполнения шага Tick
    const geom::Vec2D GetSpeed() const noexcept;
    double GetDefaultSpeed() const noexcept;
    Direction GetDirection() const noexcept;

    size_t GetBagCapacity() const noexcept;
    bool IsBagFull() const noexcept;
    const LootPtrs& GetBag() const noexcept;
    void AddLootIntoBag(LootPtr loot);

    void IncreaseScore(size_t score) noexcept;
    void UpdateScore(const Map::LootTypes& loot_types);
    size_t GetScore() const noexcept;

    void SetDirectionSpeed(std::string_view str);
    void SetPosition(geom::Point2D pos) noexcept;
    void SetSpeed(geom::Vec2D speed) noexcept;

    void IncreasePlayTime(const int time_delta) noexcept;
    void SetPlayTime(const int time_delta) noexcept;
    uint32_t GetPlayTime() const noexcept;

    void ResetInactivityTime() noexcept;
    void IncreaseInactivityTime(const int time_delta) noexcept;
    void SetInactivityTime(const int time_delta) noexcept;
    uint32_t GetInactivityTime() const noexcept;

    bool IsMoving() const noexcept;
    void Stopped() noexcept;
    void Moving() noexcept;

private:
    Id id_;
    std::string name_;
    geom::Point2D pos_ = {0.0, 0.0}; // позиция после хода
    double default_speed_;
    size_t bag_capacity_ = DEFAULT_DOG_BAG_CAPACITY;

    geom::Point2D prev_pos_ = {0.0, 0.0}; // позиция до начала хода
    geom::Vec2D speed_ = {0.0, 0.0};
    Direction dir_ = Direction::NORTH;

    size_t score_ = 0;
    uint32_t play_time_ = 0; // ms
    uint32_t inactivity_time_ = 0; // ms

    bool is_moving_ = false;

    LootPtrs bag_;
};

class GameSession {
public:
    explicit GameSession(const Map* map, double period, double probability) noexcept
        : map_{map}
        , loot_generator_(milliseconds(static_cast<int>(period * MILLISECONDS_PER_SECOND)), probability) {
    }

    void AddDog(DogPtr dog);
    void AddLoot(LootPtr loot);

    const Map* GetMap() const noexcept;
    DogPtr GetDog(Dog::Id id) noexcept;
    const DogPtrs& GetDogs() const noexcept;
    const LootPtrs& GetLoots() const noexcept;

    size_t GetDogsCount() const noexcept;
    size_t GetLootsCount() const noexcept;

    LootPtr TakeLoot(Loot::Id id);

    loot_gen::LootGenerator* GetLootGenerator() noexcept;

    void RemoveDogById(Dog::Id id);

private:
    const Map* map_;
    loot_gen::LootGenerator loot_generator_;

    DogPtrs dogs_;
    LootPtrs loots_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    Game() 
        : default_speed_(DEFAULT_DOG_SPEED)
        , default_bag_capacity_(DEFAULT_DOG_BAG_CAPACITY)
        , dog_retirement_time_in_sec_(DEFAULT_DOG_RETIREMENT_TIME_IN_SEC)
        , total_loot_count_(0)
        , total_dog_count_(0)
        , loot_period_(0.0)
        , loot_probability_(0.0) {
    }

    void AddMap(Map map);

    void SetDefaultSpeed(double speed) noexcept;
    void SetDefaultBagCapacity(size_t capacity) noexcept;
    void SetRetirementTime(double time_in_sec) noexcept;
    void SetLootConfig(double period, double probability) noexcept;

    double GetDefaultSpeed() const noexcept;
    size_t GetDefaultBagCapacity() const noexcept;
    double GetRetirementTime() const noexcept;
    double GetLootPeriod() const noexcept;
    double GetLootProbability() const noexcept;

    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;

    void AddSession(GameSessionPtr gamession);
    GameSessionPtr GetSession(const Map::Id& id) noexcept;
    size_t GetSessionsCount() const noexcept;
    bool HasSession(const Map::Id& id) const noexcept;
    const GameSessionPtrs& GetSessions() const noexcept;

    uint32_t GetTotalLootsCount() const noexcept;
    void SetTotalLootsCount(uint32_t loot_count) noexcept;
    void IncreaseTotalLootsCount() noexcept;

    uint32_t GetTotalDogsCount() const noexcept;
    void SetTotalDogsCount(uint32_t dog_count) noexcept;
    void IncreaseTotalDogsCount() noexcept;

private:
    double default_speed_;
    size_t default_bag_capacity_;
    double dog_retirement_time_in_sec_; // время в секундах
    uint32_t total_loot_count_; // число всех добавленных предметов в игре
    uint32_t total_dog_count_; // число всех добавленных собак игроков в игре
    double loot_period_; 
    double loot_probability_;

    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;

    GameSessionPtrs sessions_;

    MapIdToIndex map_id_to_session_;
};

}  // namespace model