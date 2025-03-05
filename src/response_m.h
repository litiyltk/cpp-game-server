#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "magic_defs.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string_view>
#include <string>
#include <variant>


namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;

    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;
    using FileRequestResult = std::variant<StringResponse, FileResponse>;


        StringResponse Make(http::status status,
                            std::string_view body,
                            unsigned version,
                            bool keep_alive,
                            std::string_view content_type = ContentType::TEXT_JSON,
                            std::string_view allow_field = "");

        StringResponse ReportServerError(unsigned version,
                            bool keep_alive,
                            http::status status = http::status::internal_server_error,
                            std::string_view message = ErrorMessage::INTERNAL_SERVER_ERROR);

}