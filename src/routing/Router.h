#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <regex>
#include <optional>

#include "../utils/json.hpp"

enum HTTPMethod {
    GET,
    POST,
    DELETE,
    PUT
};

enum AccessLevel {
    ADMIN = 0,
    USER = 1,
    ANON = 2,
};

struct PathSegment {
    std::string value;
    bool is_dynamic = false;
};

HTTPMethod string_to_http_method(const std::string& method_str);
AccessLevel string_to_access_level(const std::string& level_str);

struct Route {

    Route(
        std::string name,
        std::string path,
        std::string handlerScript,
        const std::set<HTTPMethod>&
        methods_to_add,
        std::set<std::string> middlewares,
        AccessLevel accessLevel
    );

    std::string name;
    std::string original_path;
    std::vector<PathSegment> parsed_path;
    std::set<HTTPMethod> methods;
    AccessLevel accessLevel;
    std::string handlerScript;
    std::set<std::string> middlewareScripts;

    [[nodiscard]] std::optional<std::unordered_map<std::string, std::string>> match(
        const std::string& requestPath,
        HTTPMethod requestMethod) const;
};

class Router {
public:
    Router();

    Route* find_route(
        const std::string& requestPath,
        HTTPMethod requestMethod,
        std::unordered_map<std::string, std::string>& out_params
    );

    void add_route(Route route);

    static Route parse_route_json(nlohmann::json& json);

    void add_route_with_json(nlohmann::json& json);

    ~Router() = default;

private:
    std::vector<std::unique_ptr<Route>> routes;
    std::unordered_map<std::string, Route*> routes_by_name;
    std::vector<std::unique_ptr<Route>> static_routes;
    std::vector<std::unique_ptr<Route>> dynamic_routes;
    std::string not_found_route_path = "/page-not-found/";
    Route* not_found_route_ptr = nullptr;
};

// helper

std::vector<PathSegment> parse_path(const std::string& path);