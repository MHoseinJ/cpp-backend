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
#include <sys/select.h>
#include "Core.h"

#include "HTTPObject.h"
#include "Log.h"
#include "../lua/LuaBinding.h"

std::atomic<bool> closeSignal(false);

void shutdown_server(const int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        closeSignal = true;
    }
}

void send_string(int sockfd, const std::string& message) {
    if (const ssize_t bytes_sent = write(sockfd, message.c_str(), message.length()); bytes_sent < 0) {
        perror("write");
    } else if (bytes_sent != static_cast<ssize_t>(message.length())) {
        backendLog("Not all bytes sent!", WARNING);
    }
}

HTTPServer::HTTPServer() {
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

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8000);

    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }
    backendLog("Socket bound: " + std::to_string(socket_fd), INFO);

    if (int wait_line_count = 5; listen(socket_fd, wait_line_count) < 0) {
        perror("listen");
        close(socket_fd);
        exit(1);
    }
}

void HTTPServer::Loop(const Router* router) const {
    std::signal(SIGINT, shutdown_server);
    std::signal(SIGTERM, shutdown_server);

    backendLog("Server started. press ctrl+c to exit.", INFO);

    while (!closeSignal) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);

        struct timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int activity = select(socket_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0) {
            if (closeSignal) break;
            perror("select");
            continue;
        }

        if (activity == 0) {
            continue;
        }

        sockaddr_in client{};
        socklen_t client_len = sizeof(client);
        const int client_fd = accept(socket_fd, reinterpret_cast<sockaddr *>(&client), &client_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);
        const uint16_t client_port = ntohs(client.sin_port);
        backendLog("Client connecting: " + std::string(client_ip) + ":" + std::to_string(client_port), INFO);

        std::vector<char> buffer(4096);
        const ssize_t bytes_received = read(client_fd, buffer.data(), buffer.size() - 1);

        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("read");
            } else {
                backendLog("Client disconnected", INFO);
            }
            close(client_fd);
            continue;
        }

        buffer[bytes_received] = '\0';
        std::string request_str(buffer.begin(), buffer.begin() + bytes_received);
        backendLog("Request: " + request_str, PRINT);

        std::string http_response;

        HTTPRequest request(request_str);

        if (const auto route = router->find_route(request.raw_url, request.method, request.path_parameters)) {

            backendLog("Route found: " + route->original_path, INFO);
            const sol::function function = Lua::GetFunction(route->handlerScript);
            if (function != sol::nil) {
                const sol::protected_function& pf = function;
                sol::protected_function_result result = pf(request);
                if (!result.valid()) {
                    sol::error err = result;
                    backendLog("[LUA] error: " + std::string(err.what()), ERROR);
                    continue;
                }
                http_response = result;
            }

        }

        send_string(client_fd, http_response);
        close(client_fd);
        backendLog("Client served and closed", INFO);
    }

    backendLog("Server shutting down", INFO);
}

HTTPServer::~HTTPServer() {
    close(socket_fd);
    backendLog("Socket closed: " + std::to_string(socket_fd), INFO);
}

bool parse_http_request(
    const std::string &raw_request,
    HTTPMethod &out_method,
    std::string &out_path,
    std::unordered_map<std::string, std::string> &out_params,
    const char *out_body
) {
    out_method = HTTPMethod::UNKNOWN;
    out_path = "";
    out_params.clear();

    size_t first_line_end = raw_request.find("\r\n");
    if (first_line_end == std::string::npos) {
        return false;
    }
    std::string request_line = raw_request.substr(0, first_line_end);

    std::stringstream ss_line(request_line);
    std::string method_str, path_with_query, http_version;

    if (!(ss_line >> method_str >> path_with_query >> http_version)) {
        return false;
    }

    out_method = string_to_http_method(method_str);
    if (out_method == HTTPMethod::UNKNOWN) {
        return false;
    }

    size_t query_pos = path_with_query.find('?');
    if (query_pos == std::string::npos) {
        out_path = url_decode(path_with_query);
    } else {
        std::string path_part = path_with_query.substr(0, query_pos);
        std::string query_string = path_with_query.substr(query_pos + 1);

        out_path = url_decode(path_part);

        std::stringstream query_ss(query_string);
        std::string param_pair;

        while (std::getline(query_ss, param_pair, '&')) {
            if (param_pair.empty()) continue;

            size_t eq_pos = param_pair.find('=');
            if (eq_pos == std::string::npos) {
                std::string key = url_decode(param_pair);
                out_params[key] = "";
            } else {
                std::string key = param_pair.substr(0, eq_pos);
                std::string value = param_pair.substr(eq_pos + 1);

                out_params[url_decode(key)] = url_decode(value);
            }
        }
    }

    // find the end of request (double CRLF)
    size_t headers_end = path_with_query.find('\r\n\r\n');
    if (headers_end == std::string::npos) {
        // it should return true cause if request has no body so we don't care about it
        return true;
    }

    // extract body
    size_t body_start = headers_end + 4; // skip CRLF
    if (body_start < raw_request.size()) { // it has a body
        if (out_body) {
            out_body = raw_request.data() + body_start;
        }
    }

    return true;
}

std::string url_decode(const std::string& encoded_string) {
    std::string decoded_string;
    decoded_string.reserve(encoded_string.length());

    for (size_t i = 0; i < encoded_string.length(); ++i) {
        if (encoded_string[i] == '%' && i + 2 < encoded_string.length()) {
            char hex1 = encoded_string[i + 1];
            char hex2 = encoded_string[i + 2];
            int hex_value = 0;

            if (hex1 >= '0' && hex1 <= '9') hex_value += (hex1 - '0') * 16;
            else if (hex1 >= 'A' && hex1 <= 'F') hex_value += (hex1 - 'A' + 10) * 16;
            else if (hex1 >= 'a' && hex1 <= 'f') hex_value += (hex1 - 'a' + 10) * 16;
            else { decoded_string += encoded_string[i]; continue; }

            if (hex2 >= '0' && hex2 <= '9') hex_value += (hex2 - '0');
            else if (hex2 >= 'A' && hex2 <= 'F') hex_value += (hex2 - 'A' + 10);
            else if (hex2 >= 'a' && hex2 <= 'f') hex_value += (hex2 - 'a' + 10);
            else { decoded_string += encoded_string[i]; continue; }

            decoded_string += static_cast<char>(hex_value);
            i += 2;
        } else if (encoded_string[i] == '+') {
            decoded_string += ' ';
        } else {
            decoded_string += encoded_string[i];
        }
    }
    backendLog(decoded_string, INFO);
    return decoded_string;
}