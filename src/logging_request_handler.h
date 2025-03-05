#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "server_logger.h"

#include <boost/beast/http.hpp>
#include <chrono>
#include <functional>
#include <string_view>


namespace server_logging {

namespace beast = boost::beast;
namespace http = beast::http;

template<class SomeRequestHandler>
class LoggingRequestHandler {

    template <typename RequestType>
    static void LogRequest(std::string_view ip, const RequestType& req) {
        server_logger::LogRequestReceived(ip, req.target(), req.method_string());
    }
 
    template <typename ResponseType>
    static void LogResponse(std::string_view ip, const ResponseType& res, const auto t_start) {
        const auto t_end = std::chrono::high_resolution_clock::now();
        const auto res_time = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
        server_logger::LogResponseSent(ip, res_time, res.result_int(), res[http::field::content_type]);
    }
 
public:
    explicit LoggingRequestHandler(SomeRequestHandler&& handler)
        : decorated_{std::move(handler)} {
    }
 
    LoggingRequestHandler(const LoggingRequestHandler&) = delete;
    LoggingRequestHandler& operator=(const LoggingRequestHandler&) = delete;
 
    template <typename Body, typename Allocator, typename Send>
    void operator()(std::string_view ip, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
 
        LogRequest(ip, req);
 
        const auto t_start = std::chrono::high_resolution_clock::now();
        auto new_send = [t_start, send, ip](auto&& res) {
            LogResponse(ip, res, t_start);
            send(std::move(res));
        };
 
        decorated_(std::forward<decltype(req)>(req), std::forward<decltype(new_send)>(new_send));
 
    }
 
private:
    SomeRequestHandler decorated_;
};

} // namespace server_logging