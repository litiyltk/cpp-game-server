#include "json_loader.h"

namespace json_loader {

void ParseRoads(const json::array& roads_array, model::Map& map) {
    for (const auto& road_val : roads_array) {
        auto road_obj = road_val.as_object();
        auto start_x = road_obj.at(KeyX0).as_int64();
        auto start_y = road_obj.at(KeyY0).as_int64();
        if (road_obj.contains(KeyX1)) { // Горизонтальная дорога
            auto end_x = road_obj.at(KeyX1).as_int64();
            map.AddRoad(model::Road(model::Road::HORIZONTAL, {static_cast<model::Coord>(start_x), static_cast<model::Coord>(start_y)}, static_cast<model::Coord>(end_x)));
        } else if (road_obj.contains(KeyY1)) { // Вертикальная дорога
            auto end_y = road_obj.at(KeyY1).as_int64();
            map.AddRoad(model::Road(model::Road::VERTICAL, {static_cast<model::Coord>(start_x), static_cast<model::Coord>(start_y)}, static_cast<model::Coord>(end_y)));
        }
    }
}

void ParseBuildings(const json::array& buildings_array, model::Map& map) {
    for (const auto& building_val : buildings_array) {
        auto building_obj = building_val.as_object();
        auto x = building_obj.at(KeyX).as_int64();
        auto y = building_obj.at(KeyY).as_int64();
        auto width = building_obj.at(KeyW).as_int64();
        auto height = building_obj.at(KeyH).as_int64();

        model::Rectangle bounds{{static_cast<model::Coord>(x), static_cast<model::Coord>(y)},
                                {static_cast<model::Dimension>(width), static_cast<model::Dimension>(height)}};
        map.AddBuilding(model::Building(bounds));
    }
}

void ParseOffices(const json::array& offices_array, model::Map& map) {
    for (const auto& office_val : offices_array) {
        auto office_obj = office_val.as_object();
        auto id = office_obj.at(KeyId).as_string().c_str();
        auto x = office_obj.at(KeyX).as_int64();
        auto y = office_obj.at(KeyY).as_int64();
        auto offset_x = office_obj.at(KeyOffsetX).as_int64();
        auto offset_y = office_obj.at(KeyOffsetY).as_int64();

        model::Point position{static_cast<model::Coord>(x), static_cast<model::Coord>(y)};
        model::Offset offset{static_cast<model::Dimension>(offset_x), static_cast<model::Dimension>(offset_y)};
        map.AddOffice(model::Office(model::Office::Id{id}, position, offset));
    }
}

void ParseLootTypes(const json::array& loots_types_array, model::Map& map) {
    for (const auto& loot_type_val : loots_types_array) {
        model::LootType loot_type;
        auto loot_type_obj = loot_type_val.as_object();

        for (const auto& [k, v] : loot_type_obj) { // Произвольные свойства - std::variant
            std::string key = std::string(k); // ключ - std::string
            if (v.is_string()) {
                loot_type.properties[key] = v.as_string().c_str();
            } else if (v.is_double()) {
                loot_type.properties[key] = v.as_double();
            } else if (v.is_int64()) {
                loot_type.properties[key] = v.as_int64();
            } else if (v.is_bool()) {
                loot_type.properties[key] = v.as_bool();
            }
        }
        map.AddLootType(loot_type);
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // 1. Загрузить содержимое файла json_path в виде строки
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    auto json_str = buffer.str();

    // 2. Распарсить строку как JSON
    auto value = json::parse(json_str);

    auto maps = value.as_object().at(KeyMaps).as_array();

    // 3. Загрузить модель игры из файла
    model::Game game;

    // скорость по умолчанию
    if (value.as_object().find(KeyDefaultDogSpeed) != value.as_object().end()) {
        auto default_dog_speed = value.as_object().at(KeyDefaultDogSpeed).as_double(); 
        game.SetDefaultSpeed(default_dog_speed);
    }

    // вместимость рюкзака по умолчанию
    if (value.as_object().find(KeyDefaultDogBagCapacity) != value.as_object().end()) {
        auto default_dog_bag_capacity = value.as_object().at(KeyDefaultDogBagCapacity).as_int64(); 
        game.SetDefaultBagCapacity(default_dog_bag_capacity);
    }

    // предельное время игрока в режиме ожидания
    if (value.as_object().find(KeyDogRetirementTime) != value.as_object().end()) {
        auto retirement_time = value.as_object().at(KeyDogRetirementTime).as_double(); 
        game.SetRetirementTime(retirement_time);
    }

    // параметры генератора предметов
    if (value.as_object().find(KeyLootGeneratorConfig) != value.as_object().end()) {
        auto period = value.as_object().at(KeyLootGeneratorConfig).as_object().at(KeyPeriod).as_double();
        auto probability = value.as_object().at(KeyLootGeneratorConfig).as_object().at(KeyProbability).as_double();
        game.SetLootConfig(period, probability);
    }

    // индивидуальные параметры карт
    for (const auto& map_val : maps) { 
        auto map_obj = map_val.as_object();
        auto map_id = map_obj.at(KeyId).as_string().c_str();
        auto map_name = map_obj.at(KeyName).as_string().c_str();

        model::Map map(model::Map::Id{map_id}, map_name);

        // скорость для текущей карты
        if (map_obj.find(KeyDogSpeed) != map_obj.end()) {
            auto dog_speed = map_obj.at(KeyDogSpeed).as_double(); 
            map.SetDogSpeed(dog_speed);
        }

        // вместимость рюкзака для текущей карты
        if (map_obj.find(KeyDogBagCapacity) != map_obj.end()) {
            auto dog_bag_capacity = map_obj.at(KeyDogBagCapacity).as_int64(); 
            map.SetDogBagCapacity(dog_bag_capacity);
        }

        // Парсим дороги, здания и офисы, возможные предметы
        if (map_obj.contains(KeyLootTypes)) {
            ParseLootTypes(map_obj.at(KeyLootTypes).as_array(), map);
        }
        if (map_obj.contains(KeyRoads)) {
            ParseRoads(map_obj.at(KeyRoads).as_array(), map);
        }
        if (map_obj.contains(KeyBuildings)) {
            ParseBuildings(map_obj.at(KeyBuildings).as_array(), map);
        }
        if (map_obj.contains(KeyOffices)) {
            ParseOffices(map_obj.at(KeyOffices).as_array(), map);
        }

        game.AddMap(std::move(map));
    }

    return game;
}

json::array GetMapsArray(const model::Game::Maps& maps) {
    json::array maps_array;
    maps_array.reserve(maps.size());
    std::transform(maps.begin(), maps.end(), std::back_inserter(maps_array),
        [] (const auto& map) {
            return json::value{
                { KeyId, *map.GetId() },
                { KeyName, map.GetName() }
            };
        }
    );
    return maps_array;
}

json::object GetMapObject(const model::Map *map) {
    json::object map_obj;
    map_obj[KeyId] = *map->GetId();
    map_obj[KeyName] = map->GetName();
    map_obj[KeyRoads] = GetRoadsArray(map); // Добавление дорог
    map_obj[KeyBuildings] = GetBuildingsArray(map); // Добавление зданий
    map_obj[KeyOffices] = GetOfficesArray(map); // Добавление офисов
    map_obj[KeyLootTypes] = GetLootTypesArray(map); // Добавление возможных типов предметов
    return map_obj;
}

json::object GetGameStateObject(const model::DogPtrs& dogs, const model::LootPtrs& loots) {
    json::object game_state_object;
    
    // Формируем объект игроков
    json::object players_object;
    for (const auto& dog : dogs) {
        json::array bag_array; // заполняем параметрами предметов из рюкзака
        for (const auto& loot : dog->GetBag()) {
            bag_array.emplace_back(json::object{
                { KeyId, *loot->id },
                { KeyType, loot->type }
            });
        }

        players_object[std::to_string(*dog->GetId())] = {
            { KeyPos, { dog->GetPosition().x, dog->GetPosition().y } },
            { KeySpeed, { dog->GetSpeed().x, dog->GetSpeed().y } },
            { KeyDir, model::DirectionToString(dog->GetDirection()) },
            { KeyBag, std::move(bag_array) },
            { KeyScore, dog->GetScore() }
        };
    }

    // Формируем объект предметов
    json::object loots_object;
    for (const auto& loot : loots) {
        loots_object[std::to_string(*loot->id)] = {
            { KeyType, loot->type },
            { KeyPos, { loot->pos.x, loot->pos.y } }
        };
    }

    game_state_object[KeyPlayers] = std::move(players_object);
    game_state_object[KeyLostObjects] = std::move(loots_object);

    return game_state_object;
}

json::object GetPlayerListObject(const model::DogPtrs& dogs) {
    json::object dogs_object;
    for (const auto& dog : dogs) {
        json::object jv_name{
            { KeyName, dog->GetName() }
        };
        dogs_object[std::to_string(*dog->GetId())] = jv_name;
    }
    return dogs_object;
}

json::array GetGameRecordsArray(const postgres::PlayersRecords records) {
    json::array records_array;
    
    for (const auto& r : records) {
        json::value record = {
            { KeyName, r.name },
            { KeyScore, r.score },
            { KeyPlayTime, static_cast<double>(r.play_time_ms) / model::MILLISECONDS_PER_SECOND }
        };
        records_array.push_back(record);
    }

    return records_array;
}

json::array GetRoadsArray(const model::Map *map) {
    const auto& roads = map->GetRoads();
    json::array roads_array;
    roads_array.reserve(roads.size());
    std::transform(roads.begin(), roads.end(), std::back_inserter(roads_array),
        [] (const auto& road) {
            json::object road_obj;
            road_obj[KeyX0] = road->GetStart().x;
            road_obj[KeyY0] = road->GetStart().y;
            if (road->IsHorizontal()) {
                road_obj[KeyX1] = road->GetEnd().x;
            } else {
                road_obj[KeyY1] = road->GetEnd().y;
            }
            return road_obj;
        }
    );
    return roads_array;
}

json::array GetBuildingsArray(const model::Map *map) {
    const auto& buildings = map->GetBuildings();
    json::array buildings_array;
    buildings_array.reserve(buildings.size());
    std::transform(buildings.begin(), buildings.end(), std::back_inserter(buildings_array),
        [] (const auto& building) {
            const auto& bounds = building.GetBounds();
            json::object building_obj;
            building_obj[KeyX] = bounds.position.x;
            building_obj[KeyY] = bounds.position.y;
            building_obj[KeyW] = bounds.size.width;
            building_obj[KeyH] = bounds.size.height;
            return building_obj;
        }
    );
    return buildings_array;
}

json::array GetOfficesArray(const model::Map *map) {
    const auto& offices = map->GetOffices();
    json::array offices_array;
    offices_array.reserve(offices.size());
    std::transform(offices.begin(), offices.end(), std::back_inserter(offices_array),
        [] (const auto& office) {
            json::object office_obj;
            office_obj[KeyId] = *office.GetId();
            office_obj[KeyX] = office.GetPosition().x;
            office_obj[KeyY] = office.GetPosition().y;
            office_obj[KeyOffsetX] = office.GetOffset().dx;
            office_obj[KeyOffsetY] = office.GetOffset().dy;
            return office_obj;
        }
    );
    return offices_array;
}

json::array GetLootTypesArray(const model::Map* map) {
    const auto& loot_types = map->GetLootTypes();
    json::array loot_types_array;
    loot_types_array.reserve(loot_types.size());

    std::transform(loot_types.begin(), loot_types.end(), std::back_inserter(loot_types_array),
        [] (const auto& loot_type) {
            json::object loot_types_obj;

            // Преобразуем произвольные свойства обратно в JSON
            for (const auto& [key, value] : loot_type.properties) {
                if (std::holds_alternative<std::string>(value)) {
                    loot_types_obj[key] = std::get<std::string>(value);
                } else if (std::holds_alternative<double>(value)) {
                    loot_types_obj[key] = std::get<double>(value);
                } else if (std::holds_alternative<int64_t>(value)) {
                    loot_types_obj[key] = std::get<int64_t>(value);
                } else if (std::holds_alternative<bool>(value)) {
                    loot_types_obj[key] = std::get<bool>(value);
                }
            }

            return loot_types_obj;
        }
    );

    return loot_types_array;
}

json::value MakeErrorJSON(std::string_view error_code, std::string_view error_message) {
    json::value jv = {
        { JsonField::CODE, error_code },
        { JsonField::MESSAGE, error_message }
    };
    return jv;
}

} // namespace json_loader