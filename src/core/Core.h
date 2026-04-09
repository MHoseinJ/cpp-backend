#pragma once

#include <string>
#include <netinet/in.h>

class HTTPServer {
private:
    int socket_fd;
    struct sockaddr_in address{};
public:
    HTTPServer();
    [[noreturn]] void Loop() const;
    ~HTTPServer();
};

// helper

void send_string(int sockfd, const std::string& message);