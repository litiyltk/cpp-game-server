#include "response_m.h"
#include "magic_defs.h"

namespace http_handler {

    StringResponse Make(http::status status,
                        std::string_view body,
                        unsigned version,
                        bool keep_alive,
                        std::string_view content_type,
                        std::string_view allow_field) //
    {
        StringResponse response{status, version};
        response.body() = body;
        response.prepare_payload(); // автоматически content-length
        response.set(http::field::content_type, content_type);
        response.set(http::field::cache_control, MiscDefs::NO_CACHE);
        if(allow_field.size()) response.set(http::field::allow, allow_field);
        response.keep_alive(keep_alive);
        return response;
    }
    
    StringResponse ReportServerError(unsigned version,
                                    bool keep_alive,
                                    http::status status,
                                    std::string_view message)
    {
        return Make(status, 
                    message, 
                    version,
                    keep_alive,
                    ContentType::PLAIN_TEXT,
                    ""
                    );
    } 
}