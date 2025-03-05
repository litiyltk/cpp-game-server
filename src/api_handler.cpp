#include "api_handler.h"


namespace http_handler {

bool IsMethodAllowed(const http::verb& current_method, const std::unordered_set<http::verb>& allowed_methods) {
    return allowed_methods.contains(current_method);
}

StringResponse GetMap(const model::Map* map, unsigned version, bool keep_alive)  {
    auto body = json::serialize(json_loader::GetMapObject(map));
    return Make(http::status::ok, body, version, keep_alive);
}

StringResponse SetMapNotFound(unsigned version, bool keep_alive) {
    auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::MAP_NOT_FOUND, ErrorMessage::MAP_NOT_FOUND));
    return Make(http::status::not_found, body, version, keep_alive);
}

StringResponse SetInvalidMethod(std::string_view message, std::string_view misc_message, unsigned version, bool keep_alive) {
    auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::INVALID_METHOD, message));
    return Make(http::status::method_not_allowed, body, version, keep_alive, ContentType::TEXT_JSON, misc_message);
}

StringResponse SetInvalidContentType(unsigned version, bool keep_alive) {
    auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::INVALID_ARGUMENT, ErrorMessage::INVALID_CONTENT_TYPE));
    return Make(http::status::bad_request, body, version, keep_alive);
}

StringResponse SetFailedParseJson(std::string_view message, unsigned version, bool keep_alive) {
    auto body = json::serialize(json_loader::MakeErrorJSON(ErrorCode::INVALID_ARGUMENT, message));
    return Make(http::status::bad_request, body, version, keep_alive);
}

StringResponse SetGameAction(app::PlayerPtr player, std::string_view direction_str, unsigned version, bool keep_alive) {
    model::DogPtr dog = player->GetSession()->GetDog(player->GetDogId());
    dog->SetDirectionSpeed(direction_str);
    if (!direction_str.empty()) {
        dog->Moving(); // метка начала движения на текущем Tick'е
    } else {
        dog->Stopped(); // метка остановки на текущем Tick'е
    }

    json::value jv = {
        { "move", model::DirectionToString(dog->GetDirection())},
    };
    auto body = json::serialize(jv);
    return Make(http::status::ok, body, version, keep_alive);
}

std::optional<app::Token> TryExtractToken(const http::request<http::string_body>& request) {
    auto auth_header = request.find(http::field::authorization);
    if (auth_header != request.end()) {
        std::string auth_value = std::string(auth_header->value());
        std::string auth_prefix = "Bearer ";
        if (auth_value.substr(0, auth_prefix.size()) == auth_prefix) {
            std::string token_str = auth_value.substr(auth_prefix.size());
            if (app::IsValidTokenString(token_str)) {
                return app::Token(token_str);
            }
        }
    }
    return std::nullopt;
}

std::optional<int> ParseQueryInt(const std::string& url, const std::string& field) {
    size_t pos = url.find("?");
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string target = field + "=";
    size_t field_pos = url.find(target, pos + 1);
    if (field_pos == std::string::npos) {
        return std::nullopt;
    }

    size_t start_pos = field_pos + target.size();
    size_t end_pos = url.find("&", start_pos);
    std::string value = url.substr(start_pos, end_pos - start_pos);

    try {
        return std::stoi(value);
    } catch (const std::exception& ex) {
        return std::nullopt;
    }
}

std::pair<int, int> GetStartAndMaxItems(const std::string& query) {
    auto start_opt = ParseQueryInt(query, "start");
    auto max_items_opt = ParseQueryInt(query, "maxItems");

    // Значения по умолчанию
    int start = start_opt.has_value() ? start_opt.value() : 0;
    int max_items = max_items_opt.has_value() ? max_items_opt.value() : 100;

    return { start, max_items };
}

// методы класса ApiRequestHandler

    StringResponse ApiRequestHandler::GetMapsList(unsigned version, bool keep_alive) const {
        auto body = json::serialize(json_loader::GetMapsArray(app_.GetMaps()));
        return Make(http::status::ok, body, version, keep_alive);
    }

    StringResponse ApiRequestHandler::SetJoinGame(const std::string& name, const model::Map::Id& map_id, unsigned version, bool keep_alive) {
        app::JoinInfo auth_info = app_.JoinGame(name, map_id);
        json::value jv = {
            { "authToken", *auth_info.token},
            { "playerId", auth_info.id }
        };
        auto body = json::serialize(jv);
        return Make(http::status::ok, body, version, keep_alive);
    }

    StringResponse ApiRequestHandler::SetTick(int time_delta, unsigned version, bool keep_alive) const {
        app_.Tick(std::chrono::milliseconds(time_delta));
        json::object jo = {};
        auto body = json::serialize(jo);
        return Make(http::status::ok, body, version, keep_alive);
    }

    StringResponse ApiRequestHandler::GetGameRecords(int start, int max_items, unsigned version, bool keep_alive) const {
        auto body = json_loader::GetGameRecordsArray(app_.GetRecords(start, max_items));
        return Make(http::status::ok, json::serialize(body), version, keep_alive);
    }

    StringResponse ApiRequestHandler::GetPlayersList(app::PlayerPtr player, unsigned version, bool keep_alive) const {
        auto jo = json_loader::GetPlayerListObject(app_.GetDogs(player));
        auto body = json::serialize(jo);
        return Make(http::status::ok, body, version, keep_alive);
    }

    StringResponse ApiRequestHandler::GetGameState(app::PlayerPtr player, unsigned version, bool keep_alive) const {
        auto jo = json_loader::GetGameStateObject(app_.GetDogs(player), app_.GetLoots(player));
        auto body = json::serialize(jo);
        return Make(http::status::ok, body, version, keep_alive);
    }

}  // namespace http_handler