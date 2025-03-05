#include "request_handler.h"


namespace http_handler {

std::string RequestHandler::UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (auto it = str.begin(); it != str.end(); ++it) {
        if (*it == '%' && it + 2 < str.end() && isxdigit(*(it + 1)) && isxdigit(*(it + 2))) {
            result += static_cast<char>(std::stoi(std::string(it + 1, it + 3), nullptr, 16));
            it += 2;
        } else if (*it == '+') {
            result += ' ';
        } else {
            result += *it;
        }
    }
    return result;
}

std::string RequestHandler::GetUrlWithoutQuery(std::string_view str) {
    auto pos = str.find('?');
    if (pos != std::string_view::npos) {
        return UrlDecode(str.substr(0, pos));
    }
    return UrlDecode(str);
}

std::string RequestHandler::GetMimeType(std::string_view str) {
    std::string res(str);
    static const std::unordered_map<std::string, std::string> mime_types {
        {".htm", "text/html"}, {".html", "text/html"}, {".css", "text/css"},
        {".txt", "text/plain"}, {".js", "text/javascript"}, {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".gif", "image/gif"},
        {".bmp", "image/bmp"}, {".ico", "image/vnd.microsoft.icon"}, {".tiff", "image/tiff"},
        {".tif", "image/tiff"}, {".svg", "image/svg+xml"}, {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);

    auto it = mime_types.find(res);
    if (it != mime_types.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

}  // namespace http_handler
