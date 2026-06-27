#include <string>
#include <unordered_map>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/err.h>

constexpr std::string_view SERVER_NAME =
    "Evade/24.0.3";

enum class HttpMethod {
    GET, POST, PUT, DELETE, UNK
};

enum class HttpStatus {
    HTTP_OK = 200,
    HTTP_NOT_FOUND = 404,
    HTTP_BAD_REQUEST = 400,
    HTTP_INTERNAL_ERR = 500
};

enum class HttpHeader {
    HTTP_CONTENT_TYPE,
    HTTP_CONTENT_LENGTH,
    HTTP_SERVER,
    HTTP_CONNECTION
};

enum class HttpContentType {
    // text
    HTTP_TEXT_CSS,
    HTTP_TEXT_CSV,
    HTTP_TEXT_EVENT_STREAM,
    HTTP_TEXT_HTML,
    HTTP_TEXT_JAVASCRIPT,
    HTTP_TEXT_PLAIN,
    HTTP_TEXT_XML,

    // media
    HTTP_IMAGE_GIF,
    HTTP_IMAGE_JPEG,
    HTTP_IMAGE_PNG,
    HTTP_IMAGE_TIFF,
    HTTP_IMAGE_VND_MICROSOFT,
    HTTP_IMAGE_XICON,
    HTTP_IMAGE_VND_DJVU,
    HTTP_IMAGE_SVG_XML,

    // multipart
    HTTP_MULTIPART_MIXED,
    HTTP_MULTIPART_ALTERNATIVE,
    HTTP_MULTIPART_RELATED,
    HTTP_MULTIPART_FORM_DATA,

    // video
    HTTP_VIDEO_MPEG,
    HTTP_VIDEO_MP4,
    HTTP_VIDEO_QUICKTIME,
    HTTP_VIDEO_X_MS_WMV,
    HTTP_VIDEO_X_MSVIDEO,
    HTTP_VIDEO_X_FLV,
    HTTP_VIDEO_WEBM,

    // application
    HTTP_APP_JAVA_ARCHIVE,
    HTTP_APP_EDI_X12,
    HTTP_APP_EDIFACT,
    HTTP_APP_OCTET_STREAM,
    HTTP_APP_OGG,
    HTTP_APP_PDF,
    HTTP_APP_XHTML_XML,
    HTTP_APP_X_SHOCKWAVE_FLASH,
    HTTP_APP_JSON,
    HTTP_APP_LD_JSON,
    HTTP_APP_XML,
    HTTP_APP_ZIP,
    HTTP_APP_X_WWW_FORM_URLENCODED
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string version;
};

namespace util {
    bool ends_with(const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }

        return str.compare(
            str.size() - suffix.size(),
            suffix.size(),
            suffix
        ) == 0;
    }
}

namespace http {
    inline HttpMethod from_string_method(const std::string& method) {
        if (method == "GET") return HttpMethod::GET;
        if (method == "POST") return HttpMethod::POST;
        if (method == "PUT") return HttpMethod::PUT;
        if (method == "DELETE") return HttpMethod::DELETE;

        return HttpMethod::UNK;
    }

    inline std::string from_header(const HttpHeader& header) {
        switch (header) {
            case HttpHeader::HTTP_CONTENT_TYPE: {
                return "Content-Type";
            }
            case HttpHeader::HTTP_CONTENT_LENGTH: {
                return "Content-Length";
            }
            case HttpHeader::HTTP_SERVER: {
                return "Server";
            }
            case HttpHeader::HTTP_CONNECTION: {
                return "Connection";
            }
        }
        throw std::runtime_error("unknown header");
    }

    inline std::string status_string(HttpStatus status) {
        switch (status) {
            case HttpStatus::HTTP_OK:
                return "200 OK";
            case HttpStatus::HTTP_NOT_FOUND:
                return "404 Not Found";
            case HttpStatus::HTTP_BAD_REQUEST:
                return "400 Bad Request";
            case HttpStatus::HTTP_INTERNAL_ERR:
                return "500 Internal Server Error";
        }
        return "500 Internal Server Error";
    }

    // false -> throw ex
    // true -> serve as text/plain
    HttpContentType from_string_ctype(const std::string& path, bool suppressive) {
        if (util::ends_with(path, ".html")) return HttpContentType::HTTP_TEXT_HTML;
        if (util::ends_with(path, ".css")) return HttpContentType::HTTP_TEXT_CSS;
        if (util::ends_with(path, ".js")) return HttpContentType::HTTP_TEXT_JAVASCRIPT;
        if (util::ends_with(path, ".png")) return HttpContentType::HTTP_IMAGE_PNG;
        if (util::ends_with(path, ".jpg") || util::ends_with(path, ".jpeg")) return HttpContentType::HTTP_IMAGE_JPEG;
        if (suppressive) return HttpContentType::HTTP_TEXT_PLAIN;
        throw std::runtime_error("unsupported content type");
    }

    std::string to_content_type(HttpContentType type) {
        switch (type) {
            case HttpContentType::HTTP_TEXT_HTML: {return "text/html"; break;}
            case HttpContentType::HTTP_TEXT_CSS: {return "text/css"; break;}
            case HttpContentType::HTTP_TEXT_JAVASCRIPT: {return "text/javascript"; break;}
            case HttpContentType::HTTP_IMAGE_PNG: {return "image/png"; break;}
            case HttpContentType::HTTP_IMAGE_JPEG: {return "image/jpeg"; break;}
            case HttpContentType::HTTP_TEXT_PLAIN: {return "text/plain"; break;}
        }
    }
}

class HttpHeaders {
private:
    std::unordered_map<std::string, std::string> m_header_map;
public:
    inline void set(HttpHeader header, const std::string& value) {
        m_header_map[http::from_header(header)] = value;
    }

    inline std::string serialize() const {
        std::stringstream result;
        for (const auto& [key, value] : m_header_map) {
            result << key << ": " << value << "\r\n";
        }
        return result.str();
    }
};

class HttpResponse {
private:
    HttpStatus m_status;
    HttpHeaders m_headers;
    std::string m_body;
public:
    explicit HttpResponse(
        const HttpStatus& status,
        const std::string& body
    ): m_status(status), m_body(body) {}    

    HttpHeaders& headers() {
        return m_headers;
    }

    inline std::string serialize() const {
        std::stringstream response;

        // e.g. HTTP/1.1 200 (OK)
        response << "HTTP/1.1 " << http::status_string(m_status) << "\r\n";
        response << m_headers.serialize();
        response << "Content-Length: " << std::to_string(m_body.size()) << "\r\n\r\n";
        response << m_body;

        return response.str();
    }
};