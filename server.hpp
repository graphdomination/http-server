#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "http.hpp"

class HttpServer {
private:
    int m_server;
    SSL_CTX* m_ssl;

    void http_handle_request(SSL* ssl, int client_fd, const HttpRequest& request) {
        switch (request.method) {
            case HttpMethod::GET: {
                std::ifstream file("." + request.path);

                if (!file) {
                    HttpResponse response(
                        HttpStatus::HTTP_NOT_FOUND,
                        "<!DOCTYPE html>"
                        "<html>"
                        "<head>"
                                "<h1>404 (Page not found).</h1>"
                                "<p>Contact your server administrator to find out why this happened.</p>"
                                "<hr>"
                                "<title>404</title>"
                                "<address>Evade/24.0.3</address>"
                                "<style>h1 {text-align: center;}p {text-align: center;}"
                                "</style>"
                            "</head>"
                        "</html>"
                    );

                    response.headers().set(
                        HttpHeader::HTTP_CONTENT_TYPE,
                        "text/html"
                    );

                    auto data = response.serialize();
                    SSL_write(ssl, data.data(), data.size());
                    close(client_fd);
                    return;
                }

                std::string body(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>()
                );

                HttpResponse response(
                    HttpStatus::HTTP_OK,
                    body
                );

                HttpContentType type = http::from_string_ctype(request.path, false);

                response.headers().set(
                    HttpHeader::HTTP_CONTENT_TYPE,
                    http::to_content_type(type)
                );

                auto data = response.serialize();
                SSL_write(ssl, data.data(), data.size());
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(client_fd);
                break;
            }
        }
    }

    void http_handle_client(SSL* ssl, int client_fd) {
        char buf[1024];
        int bytes = SSL_read(ssl, buf, sizeof(buf) - 1);

        if (bytes <= 0) {
            close(client_fd);
            return;
        }

        buf[bytes] = '\0';
        std::string request(buf, bytes);
        std::stringstream ss(request);

        //std::cout << request << "\n";

        std::string method, path, version;
        ss >> method >> path >> version;

        http_handle_request(ssl, client_fd, {
            .method = http::from_string_method(method),
            .path = path,
            .version = version
        });
    }

public:
    HttpServer(int port) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        m_server = socket(AF_INET,  SOCK_STREAM, 0);

        int opt = 1;
        setsockopt(
            m_server,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        );

        if (m_server < 0) {
            throw std::runtime_error("failed to create TCP socket");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(m_server, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            throw std::runtime_error("failed to bind socket");
        }

        if (listen(m_server, 5) < 0) {
            throw std::runtime_error("listen");
        }

        std ::cout << "listening on " << port << std::endl;

        m_ssl = SSL_CTX_new(TLS_server_method());
        if (!m_ssl) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("failed to create ssl context");
        }

        if (SSL_CTX_use_certificate_file(m_ssl, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("cert");
        }

        if (SSL_CTX_use_PrivateKey_file(m_ssl, "key.pem", SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("key");
        }

        while (true) {
            int client_fd = accept(m_server, nullptr, nullptr);

            if (client_fd < 0) {
                throw std::runtime_error("accept");
            }

            SSL* ssl = SSL_new(m_ssl);
            SSL_set_fd(ssl, client_fd);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                close(client_fd);
                continue;
            }

            http_handle_client(ssl, client_fd);
        }

        
        close(m_server);
    }
};