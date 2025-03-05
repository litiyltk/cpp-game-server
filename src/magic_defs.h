#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <string_view>
#include <string>


using namespace std::literals;

namespace json_constants {

    static inline const std::string KeyMaps = "maps";
    static inline const std::string KeyDefaultDogSpeed = "defaultDogSpeed";
    static inline const std::string KeyDefaultDogBagCapacity = "defaultBagCapacity";
    static inline const std::string KeyLootGeneratorConfig = "lootGeneratorConfig";
    static inline const std::string KeyPeriod = "period";
    static inline const std::string KeyProbability = "probability";
    static inline const std::string KeyDogRetirementTime = "dogRetirementTime";
    static inline const std::string KeyDogSpeed = "dogSpeed";
    static inline const std::string KeyDogBagCapacity = "bagCapacity";
    static inline const std::string KeyId = "id";
    static inline const std::string KeyName = "name";
    static inline const std::string KeyType = "type";
    static inline const std::string KeyPos = "pos";
    static inline const std::string KeySpeed = "speed";
    static inline const std::string KeyDir = "dir";
    static inline const std::string KeyBag = "bag";
    static inline const std::string KeyLostObjects = "lostObjects";
    static inline const std::string KeyPlayers = "players";
    static inline const std::string KeyPlayTime = "playTime";
    static inline const std::string KeyScore = "score";
    static inline const std::string KeyLootTypes = "lootTypes";
    static inline const std::string KeyRoads = "roads";
    static inline const std::string KeyBuildings = "buildings";
    static inline const std::string KeyOffices = "offices";
    static inline const std::string KeyX0 = "x0";
    static inline const std::string KeyY0 = "y0";
    static inline const std::string KeyX1 = "x1";
    static inline const std::string KeyY1 = "y1";
    static inline const std::string KeyX = "x";
    static inline const std::string KeyY = "y";
    static inline const std::string KeyW = "w";
    static inline const std::string KeyH = "h";
    static inline const std::string KeyOffsetX = "offsetX";
    static inline const std::string KeyOffsetY = "offsetY";

} // namespace json_constants

struct ServerMessage
{
    static inline constexpr std::string_view START = "server started"sv;
    static inline constexpr std::string_view EXIT = "server exited"sv;
    static inline constexpr std::string_view REQUEST_RECEIVED = "request received"sv;
    static inline constexpr std::string_view RESPONSE_SENT = "response sent"sv;
};

struct ServerAction
{
    static inline constexpr std::string_view READ = "read"sv;
    static inline constexpr std::string_view WRITE = "write"sv;
    static inline constexpr std::string_view ACCEPT = "accept"sv;
};

struct JsonField
{
    static inline const std::string CODE = "code"s;
    static inline const std::string TEXT = "text"s;
    static inline const std::string PORT = "port"s;
    static inline const std::string DATA = "data"s;
    static inline const std::string NuLL = "null"s;
    static inline const std::string ADDRESS = "address"s;
    static inline const std::string MESSAGE = "message"s;
    static inline const std::string EXCEPTION = "exception"s;
    static inline const std::string WHERE = "where"s;
    static inline const std::string ERROR = "error"s;
    static inline const std::string IP = "ip"s;
    static inline const std::string URI = "URI"s;
    static inline const std::string METHOD = "method"s;
    static inline const std::string RESPONSE_TIME = "response_time"s;
    static inline const std::string TIMESTAMP = "timestamp"s;
    static inline const std::string CONTENT_TYPE = "content_type"s;
};

struct ServerParam
{
    static inline constexpr std::string_view ADDR = "0.0.0.0"sv;
    static inline const uint32_t PORT = 8080;
};

struct ErrorCode
{
    static inline constexpr std::string_view BAD_REQUEST = "badRequest"sv;
    static inline constexpr std::string_view INVALID_METHOD = "invalidMethod"sv;
    static inline constexpr std::string_view INVALID_ARGUMENT = "invalidArgument"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "invalidToken"sv;
    static inline constexpr std::string_view MAP_NOT_FOUND = "mapNotFound"sv;
    static inline constexpr std::string_view FILE_NOT_FOUND = "fileNotFound"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "unknownToken"sv;
};


struct ErrorMessage
{
    static inline constexpr std::string_view BAD_REQUEST = "Bad request"sv;
    static inline constexpr std::string_view INVALID_ENDPOINT = "Invalid endpoint"sv;
    static inline constexpr std::string_view POST_IS_EXPECTED = "Only POST method is expected"sv;
    static inline constexpr std::string_view GET_IS_EXPECTED = "Only GET method is expected"sv;
    static inline constexpr std::string_view FAILED_TO_PARSE_JOIN_GAME = "Join game request parse error"sv;
    static inline constexpr std::string_view INVALID_NAME = "Invalid name"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "Authorization header is missing"sv;
    static inline constexpr std::string_view MAP_NOT_FOUND = "Map not found"sv;
    static inline constexpr std::string_view FILE_NOT_FOUND = "File not found"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "Player token has not been found"sv;
    static inline constexpr std::string_view UNKNOWN_SERVER_CONFIG = "Usage: bin/game_server --config-file <game-config-json> --www-root <static-files-root>"sv;
    static inline constexpr std::string_view INVALID_STATIC_ROOT = "Invalid static files root path"sv;
    static inline constexpr std::string_view INVALID_PATH = "Path is invalid"sv;
    static inline constexpr std::string_view INTERNAL_SERVER_ERROR = "Internal server error"sv;
    static inline constexpr std::string_view FAILED_TO_PARSE_ACTION = "Failed to parse action"sv;
    static inline constexpr std::string_view FAILED_TO_PARSE_TICK = "Failed to parse tick request JSON"sv;
    static inline constexpr std::string_view INVALID_CONTENT_TYPE = "Invalid content type"sv;
};

struct MiscDefs
{
    static inline constexpr std::string_view NO_CACHE = "no-cache"sv;
};

struct MiscMessage
{
    static inline constexpr std::string_view ALLOWED_POST_METHOD = "POST"sv;
    static inline constexpr std::string_view ALLOWED_GET_HEAD_METHOD = "GET, HEAD"sv;
};

struct Endpoint
{
    static inline constexpr std::string_view API = "/api/"sv;
    static inline constexpr std::string_view MAPS = "/api/v1/maps"sv;
    static inline constexpr std::string_view MAP = "/api/v1/maps/"sv;
    static inline constexpr std::string_view GAME = "/api/v1/game/"sv;
    static inline constexpr std::string_view JOIN_GAME = "/api/v1/game/join"sv;
    static inline constexpr std::string_view PLAYERS_LIST = "/api/v1/game/players"sv;
    static inline constexpr std::string_view GAME_STATE = "/api/v1/game/state"sv;
    static inline constexpr std::string_view GAME_ACTION = "/api/v1/game/player/action"sv;
    static inline constexpr std::string_view GAME_TICK = "/api/v1/game/tick"sv;
    static inline constexpr std::string_view GAME_RECORDS = "/api/v1/game/records"sv;
};

struct ContentType
{
    ContentType() = delete;
    constexpr static std::string_view TEXT_JSON = "application/json";
    constexpr static std::string_view PLAIN_TEXT = "text/plain";
    constexpr static std::string_view HTML_TEXT = "text/html";
    constexpr static std::string_view OCTET_STREAM = "application/octet-stream";
};