#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "app.h"
#include "api_handler.h"
#include "http_server.h"
#include "json_loader.h"
#include "magic_defs.h"
#include "response_m.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>


namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;
namespace net = boost::asio;

using namespace json_constants;

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    
    RequestHandler(const fs::path static_root, Strand api_strand, app::Application& app)
        : static_root_{fs::weakly_canonical(std::move(static_root))}
        , api_strand_{api_strand}
        , api_handler_{app} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        try {
            if (api_handler_.IsApiRequest(req)) {
                auto handle = [self = shared_from_this(), send,
                               req = std::forward<decltype(req)>(req), version, keep_alive] {
                    try {
                        // лямбда-функция будет выполняться внутри strand
                        return send(self->api_handler_.HandleApiRequest(req));
                    } catch (const std::exception& ex) {
                        server_logger::LogServerStop(EXIT_FAILURE, "Error by request to API: "s + ex.what());
                        send(ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                HandleFileRequest(req));
        } catch (const std::exception& ex) {
            server_logger::LogServerStop(EXIT_FAILURE, "Error by file request: "s + ex.what());
            send(ReportServerError(version, keep_alive));
        }
    }

private:
    fs::path static_root_;
    Strand api_strand_;
    ApiRequestHandler api_handler_;

    template <typename Body, typename Allocator>
    FileRequestResult HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        fs::path path = fs::weakly_canonical(static_root_ / GetUrlWithoutQuery(req.target()).substr(1));

        if (path.string().find(static_root_) != 0) { // ошибка пути 400, "text/plain"
            return ReportServerError(version, keep_alive, http::status::bad_request, ErrorMessage::INVALID_PATH);
        }

        if (fs::is_directory(path)) { // корневая директория
            path /= "index.html";
        }

        if (!fs::exists(path) || !fs::is_regular_file(path)) { // файл не найден 404, "text/plain"
            return ReportServerError(version, keep_alive, http::status::not_found, ErrorMessage::FILE_NOT_FOUND);
        }

        http::file_body::value_type file;
        if (sys::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec) { // ошибка открытия файла 500, "text/plain"
            return ReportServerError(version, keep_alive);
        }

        // Создание ответа с содержимым файла
        FileResponse response{http::status::ok, version};
        response.set(http::field::content_type, GetMimeType(path.extension().string()));
        response.set(http::field::cache_control, MiscDefs::NO_CACHE);
        response.keep_alive(keep_alive);
        response.body() = std::move(file);
        response.prepare_payload();
        return response;
    }

    std::string UrlDecode(std::string_view str);
    std::string GetUrlWithoutQuery(std::string_view str);
    std::string GetMimeType(std::string_view str);
};

}  // namespace http_handler