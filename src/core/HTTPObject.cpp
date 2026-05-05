#include "HTTPObject.h"
#include <utility>
#include "Core.h"
#include "Log.h"

HTTPRequest::HTTPRequest(std::string raw_request) : raw_data(std::move(raw_request)) {
    parse_http_request(raw_data, method, raw_url, query_parameters, body);
}

std::string HTTPRequest::get_path_parameters(const std::string& key) {
    auto it = path_parameters.find(key);
    if (it == path_parameters.end()) {
        backendLog("there is no object with the key: " + key, ERROR);
        return "";
    }

    return it->second;
}

std::string HTTPRequest::get_query_parameters(const std::string &key) {
    const auto it = query_parameters.find(key);
    if (it == query_parameters.end()) {
        backendLog("there is no object with the key: " + key, ERROR);
        return "";
    }

    return it->second;
}
