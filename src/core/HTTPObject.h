#pragma once
#include <string>
#include <unordered_map>

#include "../routing/Router.h"

class HTTPRequest {

public:

    std::string raw_data;
    std::string http_version;
    HTTPMethod method;
    std::string raw_url;

    std::unordered_map<std::string, std::string> path_parameters;
    std::unordered_map<std::string, std::string> query_parameters;
    std::unordered_map<std::string, std::string> headers;

    std::string get_path_parameters(const std::string& key);
    std::string get_query_parameters(const std::string& key);

    const char* body;

    explicit HTTPRequest(std::string raw_request);
};
