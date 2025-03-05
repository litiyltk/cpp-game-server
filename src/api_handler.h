#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "app.h"
#include "json_loader.h"
#include "magic_defs.h"
#include "model.h"
#include "response_m.h"
#include "tagged.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <optional>
#include <string_view>


namespace http_handler {

constexpr int LIMIT_MAX_RECORDS_ITEMS = 100;

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;

bool IsMethodAllowed(const http::verb& current_method, const std::unordered_set<http::verb>& allowed_methods);
StringResponse GetMap(const model::Map* map, unsigned version, bool keep_alive);
StringResponse SetMapNotFound(unsigned version, bool keep_alive);
StringResponse SetInvalidMethod(std::string_view message, std::string_view misc_message, unsigned version, bool keep_alive);
StringResponse SetInvalidContentType(unsigned version, bool keep_alive);
StringResponse SetFailedParseJson(std::string_view message, unsigned version, bool keep_alive);
StringResponse SetGameAction(app::PlayerPtr player, std::string_view direction_str, unsigned version, bool keep_alive);
std::optional<app::Token> TryExtractToken(const http::request<http::string_body>& request);
std::optional<int> ParseQueryInt(const std::string& url, const std::string& field);
std::pair<int, int> GetStartAndMaxItems(const std::string& query);

class ApiRequestHandler {
public:
    explicit ApiRequestHandler(app::Application& app)
        : app_{app} {
    }

    template <typename Body, typename Allocator>
    StringResponse HandleApiRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

    // запросы без авторизации //

        // 1. ListMaps
        if (req.target() == Endpoint::MAPS) { // успешный запрос ../maps
            if (!(IsMethodAllowed(req.method(), {http::verb::get, http::verb::head}))) { // метод отличается от GET или HEAD
                return SetInvalidMethod(ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD, version, keep_alive);
            }
            return GetMapsList(version, keep_alive);
        }

        // 2. FindMap
        if (req.target().starts_with(Endpoint::MAP)) { // запрос ../api/v1/maps/..
            if (!(IsMethodAllowed(req.method(), {http::verb::get, http::verb::head}))) { // метод отличается от GET или HEAD
                return SetInvalidMethod(ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD, version, keep_alive);
            }

            std::string map_id_str{req.target().substr(Endpoint::MAP.size())}; // парсинг запроса на ID карты
            model::Map::Id map_id(map_id_str);

            if (auto map = app_.FindMap(map_id)) { // успех: карта найдена 200
                return GetMap(map, version, keep_alive);
            }

            return SetMapNotFound(version, keep_alive); // карта не найдена 404
        }

        // 3. JoinGame    
        if (req.target() == Endpoint::JOIN_GAME) { // запрос ../api/v1/game/join
            if (!(IsMethodAllowed(req.method(), {http::verb::post}))) { // метод отличается от POST
                return SetInvalidMethod(ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD, version, keep_alive);
            }

            json::error_code ec;
            auto value = json::parse(req.body(), ec);
            if (ec || !value.as_object().contains("userName") || !value.as_object().contains("mapId")) { // ошибка парсинга JSON
                return SetFailedParseJson(ErrorMessage::FAILED_TO_PARSE_JOIN_GAME, version, keep_alive);
            }
          
            std::string name = value.as_object().at("userName").as_string().c_str();
            if (name.empty()) { // пустое имя игрока
                return SetFailedParseJson(ErrorMessage::INVALID_NAME, version, keep_alive);
            }

            model::Map::Id map_id = model::Map::Id{value.as_object().at("mapId").as_string().c_str()};
            if (app_.FindMap(map_id) == nullptr) { // карта не найдена
                return SetMapNotFound(version, keep_alive);
            }

            return SetJoinGame(name, map_id, version, keep_alive); // успех - добавляем игрока
        }

        // 4. Tick
        if (req.target() == Endpoint::GAME_TICK) { // запрос ../api/v1/game/tick
            if (app_.HasTickPeriod()) { // задан период, запрос некорректный
                auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::BAD_REQUEST, ErrorMessage::INVALID_ENDPOINT)); // некорректный запрос 400
                return Make(http::status::bad_request, body, version, keep_alive);
            }

            if (!(IsMethodAllowed(req.method(), {http::verb::post}))) { // метод отличается от POST
                return SetInvalidMethod(ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD, version, keep_alive);
            }

            auto content_type_header = req.find(http::field::content_type);
            if (content_type_header == req.end() || content_type_header->value() != ContentType::TEXT_JSON) { // Content-Type отсутствует или отличается от "application/json"
                return SetInvalidContentType(version, keep_alive);
            }

            json::error_code ec;
            auto value = json::parse(req.body(), ec);

            // Проверка на ошибку парсинга JSON или отсутствие поля timeDelta
            if (ec || !value.is_object() || !value.as_object().contains("timeDelta")) {
                return SetFailedParseJson(ErrorMessage::FAILED_TO_PARSE_TICK, version, keep_alive);
            }

            // Проверка на валидность значения timeDelta
            auto time_delta_value = value.as_object().at("timeDelta");

            if (!time_delta_value.is_int64() || time_delta_value.as_int64() == 0) {
                return SetFailedParseJson(ErrorMessage::FAILED_TO_PARSE_TICK, version, keep_alive);
            }
 
            return SetTick(time_delta_value.as_int64(), version, keep_alive); // успех - выполняем Tick
        }

        // 5. GameRecords
        if (req.target().starts_with(Endpoint::GAME_RECORDS)) { // успешный запрос ../api/v1/game/records
            if (!(IsMethodAllowed(req.method(), {http::verb::get, http::verb::head}))) { // метод отличается от GET или HEAD
                return SetInvalidMethod(ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD, version, keep_alive);
            }
            
            // получаем параметры для среза рекордов
            auto [start, max_items] = GetStartAndMaxItems(std::string(req.target()));

            // валидный случай
            if (max_items <= LIMIT_MAX_RECORDS_ITEMS) { // успех - возвращаем массив рекордов
                return GetGameRecords(start, max_items, version, keep_alive);
            }
        }

    // запросы, требующие авторизации //
        
        // 6. PlayersList
        if (req.target() == Endpoint::PLAYERS_LIST) { // запрос ../api/v1/game/players
            if (!(IsMethodAllowed(req.method(), {http::verb::get, http::verb::head}))) { // метод отличается от GET или HEAD
                return SetInvalidMethod(ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD, version, keep_alive);
            }

            return HandleWithAuthorization(req, [this](app::PlayerPtr player, auto version, auto keep_alive) {
                return GetPlayersList(player, version, keep_alive); // успех
            });
        }

        // 7. GameState
        if (req.target() == Endpoint::GAME_STATE) { // запрос ../api/v1/game/state
            if (!(IsMethodAllowed(req.method(), {http::verb::get, http::verb::head}))) { // метод отличается от GET или HEAD
                return SetInvalidMethod(ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD, version, keep_alive);
            }

            return HandleWithAuthorization(req, [this](app::PlayerPtr player, auto version, auto keep_alive) {
                return GetGameState(player, version, keep_alive); // успех
            });
        }

        // 8. GameAction
        if (req.target() == Endpoint::GAME_ACTION) { // запрос ../api/v1/game/action
            if (!(IsMethodAllowed(req.method(), {http::verb::post}))) { // метод отличается от POST
                return SetInvalidMethod(ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD, version, keep_alive);
            }

            auto content_type_header = req.find(http::field::content_type);
            if (content_type_header == req.end() || content_type_header->value() != ContentType::TEXT_JSON) { // Content-Type отсутствует или отличается от "application/json"
                return SetInvalidContentType(version, keep_alive);
            }

            json::error_code ec;
            auto value = json::parse(req.body(), ec);
            if (ec || !value.as_object().contains("move")) { // ошибка парсинга JSON
                return SetFailedParseJson(ErrorMessage::FAILED_TO_PARSE_ACTION, version, keep_alive);
            }
          
            std::string_view direction_str = value.as_object().at("move").as_string(); // ошибка парсинга JSON, некорректное поле move
            if (!model::IsValidDirection(direction_str)) {
                return SetFailedParseJson(ErrorMessage::FAILED_TO_PARSE_ACTION, version, keep_alive);
            }

            return HandleWithAuthorization(req, [this, direction_str](app::PlayerPtr player, auto version, auto keep_alive) {
                return SetGameAction(player, direction_str, version, keep_alive); // успех
            });
        }

    // некорректный запрос 400
        auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::BAD_REQUEST, ErrorMessage::BAD_REQUEST));
        return Make(http::status::bad_request, body, version, keep_alive);
    }

    template <typename Body, typename Allocator>
    bool IsApiRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
        return req.target().starts_with(Endpoint::API);
    }

private:
    app::Application& app_;

    StringResponse GetMapsList(unsigned version, bool keep_alive) const;
    StringResponse SetJoinGame(const std::string& name, const model::Map::Id& map_id, unsigned version, bool keep_alive);
    StringResponse SetTick(int time_delta, unsigned version, bool keep_alive) const;
    StringResponse GetGameRecords(int start, int max_items, unsigned version, bool keep_alive) const;
    StringResponse GetPlayersList(app::PlayerPtr player, unsigned version, bool keep_alive) const;
    StringResponse GetGameState(app::PlayerPtr player, unsigned version, bool keep_alive) const;

    template <typename Body, typename Allocator, typename Handler>
    StringResponse HandleWithAuthorization(const http::request<Body, http::basic_fields<Allocator>>& req, Handler handler) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        if (const auto token_opt = TryExtractToken(req)) { // извлекаем токен
            const auto token = token_opt.value();
            if (app::PlayerPtr player = app_.FindPlayerByToken(token)) { // токен извлечён и найден
                return handler(player, version, keep_alive);
            } else { // токен извлечён, но не найден
                auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::UNKNOWN_TOKEN, ErrorMessage::UNKNOWN_TOKEN));
                return Make(http::status::unauthorized, body, version, keep_alive, ContentType::TEXT_JSON);
            }
        }

        // токен не удалось извлечь, некорректный заголовок авторизации
        auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::INVALID_TOKEN, ErrorMessage::INVALID_TOKEN));
        return Make(http::status::unauthorized, body, version, keep_alive);
    }
};

}  // namespace http_handler