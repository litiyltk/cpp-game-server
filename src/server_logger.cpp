#include "server_logger.h"


namespace server_logger {

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    auto message = rec[logging::expressions::smessage].get();
    auto data = rec[additional_data].get();

    json::value log_json = {
        {JsonField::TIMESTAMP, to_iso_extended_string(*ts)},
        {JsonField::DATA, data},
        {JsonField::MESSAGE, message}
    };

    strm << json::serialize(log_json) << std::endl;
}

void InitBoostLog() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

void LogMessage(const json::value& data, std::string_view message) {
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                            << message;
}

void LogServerStart(std::string_view address, size_t port) {
    json::value start_data = {
        {JsonField::PORT, port},
        {JsonField::ADDRESS, address}
    };
    
    LogMessage(start_data, ServerMessage::START);
}

void LogServerStop(int code, std::string_view exception) {
    json::value stop_data;
    if (exception.empty()) {
        stop_data = {
            {JsonField::CODE, code}
        };
    } else {
        stop_data = {
            {JsonField::CODE, code},
            {JsonField::EXCEPTION, exception} 
        };
    }

    LogMessage(stop_data, ServerMessage::EXIT);
}

void LogServerError(beast::error_code ec, std::string_view where) {
    json::value error_data = {
        {JsonField::CODE, ec.value()},
        {JsonField::TEXT, ec.message()},
        {JsonField::WHERE, where}
    };

    LogMessage(error_data, JsonField::ERROR);
}

void LogRequestReceived(std::string_view client_ip, std::string_view uri, std::string_view method) {
    boost::json::value request_data = {
        {JsonField::IP, client_ip},
        {JsonField::URI, uri},
        {JsonField::METHOD, method}
    };

    LogMessage(request_data, ServerMessage::REQUEST_RECEIVED);
}

void LogResponseSent(std::string_view client_ip, int response_time, int code, std::string_view content_type) {
    boost::json::value log_data = {
        {JsonField::IP, std::string(client_ip)},
        {JsonField::RESPONSE_TIME, response_time},
        {JsonField::CODE, code},
        {JsonField::CONTENT_TYPE, content_type.empty() ? JsonField::NuLL : content_type}
    };

    LogMessage(log_data, ServerMessage::RESPONSE_SENT);
}

} // namespace server_logger