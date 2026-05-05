#pragma once

#include <string>
#include <atomic>
#include <netinet/in.h>

#include "../routing/Router.h"

extern std::atomic<bool> closeSignal;

class HTTPServer {
private:
    int socket_fd;
    struct sockaddr_in address{};
public:
    HTTPServer();
    void Loop(Router* router) const;
    ~HTTPServer();
};

// helper

void send_string(int sockfd, const std::string& message);
void shutdown_server(int signal);
bool parse_http_request(
    const std::string &raw_request,
    HTTPMethod &out_method,
    std::string &out_path,
    std::unordered_map<std::string, std::string> &out_params,
    const char *out_body
);
std::string url_decode(const std::string& encoded_string);