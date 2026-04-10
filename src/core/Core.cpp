#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <string>
#include <atomic>
#include <csignal>

#include "Core.h"
#include "Log.h"


std::atomic<bool> closeSignal(false);

HTTPServer::HTTPServer() {
    // create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(1);
    }

    backendLog("Socket created: " + std::to_string(socket_fd), INFO);

    constexpr int optval = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        exit(1);
    }

    // fill with zero
    memset(&address, 0, sizeof(address));

    // set to ipv4
    address.sin_family = AF_INET;
    // allow all interfaces
    address.sin_addr.s_addr = INADDR_ANY;

    // set port to 8000
    address.sin_port = htons(8000);

    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }
    backendLog("Socket bound: " + std::to_string(socket_fd), INFO);

    // listening for connections
    int wait_line_count = 5;
    if (listen(socket_fd, wait_line_count) < 0) {
        perror("listen");
        close(socket_fd);
        exit(1);
    }
}

void HTTPServer::Loop() const {
    std::signal(SIGINT, shutdown_server);

    backendLog("Server started. press ctrl+c to exit.", INFO);

    while (!closeSignal) {
        
        if (closeSignal) {
            break;
        }
        // accept input connections
        struct sockaddr_in client{};
        socklen_t client_len = sizeof(client);

        int client_fd = accept4(socket_fd, reinterpret_cast<sockaddr *>(&client), &client_len, SOCK_NONBLOCK); // TODO: fix late response to exit
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                backendLog("Client disconnected.", INFO);
                sleep(1);
            }
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);

        const uint16_t client_port = ntohs(client.sin_port);

        backendLog("Client connecting: " + std::string(client_ip) + ":" + std::to_string(client_port), INFO);
        backendLog("Client connected: " + std::to_string(socket_fd), INFO);

        std::vector<char> buffer(1024);
        const ssize_t bytes_recived = read(client_fd, buffer.data(), buffer.size() - 1);
        if (bytes_recived == 0) {
            backendLog("Client disconnected", INFO);
            close(client_fd);
            continue;
        } else if (bytes_recived < 0) {
            perror("read");
            close(client_fd);
            continue;
        }

        buffer[bytes_recived] = '\0';

        std::string request_str = std::string(buffer.begin(), buffer.begin() + bytes_recived);
        backendLog("Request: " + request_str, PRINT);

        std::string http_response;

        if (request_str.rfind("GET / ", 0) == 0) {
            http_response = "HTTP/1.1 200 OK\r\n";
            http_response += "Content-Type: text/html; charset=utf-8\r\n";
            http_response += "Connection: close\r\n";
            http_response += "\r\n";
            http_response += "<html><head><meta charset=\"UTF-8\"></head><body>";
            http_response += "<h1>HELLO!</h1>";
            http_response += "<p>it's just a simple response</p>";
            http_response += "</body></html>";
        } else {
            http_response = "HTTP/1.1 404 Not Found\r\n";
            http_response += "Content-Type: text/html; charset=utf-8\r\n";
            http_response += "Connection: close\r\n";
            http_response += "\r\n";
            http_response += "<html><head><meta charset=\"UTF-8\"></head><body><h1>no page found</h1></body></html>";
        }

        send_string(client_fd, http_response);

        close(client_fd);
        backendLog("Closing client", INFO);
    }
    backendLog("reached here", INFO);
}

HTTPServer::~HTTPServer() {

    // close the socket
    close(socket_fd);

    backendLog("Socket closed: " + std::to_string(socket_fd), INFO);
}

// helper

void send_string(int sockfd, const std::string& message) {
    ssize_t bytes_sent = write(sockfd, message.c_str(), message.length());
    if (bytes_sent < 0) {
        perror("write");
    } else if (bytes_sent != message.length()) {
        backendLog("Not all bytes sent!", WARNING);
    }
}

void shutdown_server(const int signal) {
    if (signal == SIGINT) {
        closeSignal = true;
        backendLog("Closing server", INFO);
    }
}