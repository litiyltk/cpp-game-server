#pragma once

#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "magic_defs.h"

#include <boost/beast/core/error.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <string_view>


namespace server_logger {

namespace json = boost::json;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace beast = boost::beast;

using namespace std::literals;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
void InitBoostLog();

void LogMessage(const boost::json::value& data, std::string_view message);
void LogServerStart(std::string_view address, size_t port);
void LogServerStop(int code, std::string_view exception = "");
void LogServerError(beast::error_code ec, std::string_view where);
void LogRequestReceived(std::string_view client_ip, std::string_view uri, std::string_view method);
void LogResponseSent(std::string_view client_ip, int response_time, int code, std::string_view content_type = "");

} // namespace server_logger