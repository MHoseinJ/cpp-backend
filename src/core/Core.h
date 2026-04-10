#pragma once

#include <string>
#include <atomic>
#include <netinet/in.h>

extern std::atomic<bool> closeSignal;

class HTTPServer {
private:
    int socket_fd;
    struct sockaddr_in address{};
public:
    HTTPServer();
    void Loop() const;
    ~HTTPServer();
};

// helper

void send_string(int sockfd, const std::string& message);
void shutdown_server(int signal);