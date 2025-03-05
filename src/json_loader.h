#pragma once

#include "magic_defs.h"
#include "model.h"
#include "postgres.h"

#include <boost/json.hpp>
#include <filesystem>
#include <fstream> 
#include <variant> // для Properties для LootType
#include <stdexcept> 
#include <string>
#include <sstream> 

namespace json = boost::json;
using namespace std::literals;
using namespace json_constants;


namespace json_loader {

// функции для парсинга данных из файла конфига

void ParseRoads(const json::array& roads_array, model::Map& map);
void ParseBuildings(const json::array& buildings_array, model::Map& map);
void ParseOffices(const json::array& offices_array, model::Map& map);

model::Game LoadGame(const std::filesystem::path& json_path);

// функции для обработки запросов к API

json::array GetMapsArray(const model::Game::Maps& maps); // метод для запроса /api/v1/maps
json::object GetMapObject(const model::Map *map); // метод для запроса /api/v1/maps/<mapX>
json::object GetGameStateObject(const model::DogPtrs& dogs, const model::LootPtrs& loots); // метод для запроса /api/v1/game/state
json::object GetPlayerListObject(const model::DogPtrs& dogs); // метод для запроса /api/v1/game/players
json::array GetGameRecordsArray(const postgres::PlayersRecords records); // метод для запроса /api/v1/game/records

// вспомогательный функции

json::array GetRoadsArray(const model::Map *map);
json::array GetBuildingsArray(const model::Map *map);
json::array GetOfficesArray(const model::Map *map);
json::array GetLootTypesArray(const model::Map *map);

json::value MakeErrorJSON(std::string_view error_code, std::string_view error_message);

}  // namespace json_loader
