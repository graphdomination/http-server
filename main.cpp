#include "server.hpp"

#include <openssl/ssl.h>

int main() {
    std::cout << OpenSSL_version(OPENSSL_VERSION) << std::endl;
    int port = 8082;
    HttpServer http(port);
    return 0;
}