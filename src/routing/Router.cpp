#include "Router.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "../core/Log.h"
#include "../utils/FileSystem.h"

std::vector<PathSegment> parse_path(const std::string& path) {
    std::vector<PathSegment> path_segments;
    std::string normalized_path = path;

    // Handle root path explicitly
    if (normalized_path == "/") {
        // Represent root path as a single, potentially empty, segment or handle specially.
        // For simplicity here, let's ensure it doesn't result in an empty vector if it's the only thing.
        // We can add a dummy segment or adjust logic downstream.
        // A common approach is to treat "/" as having one segment, which is empty.
        // Let's adjust the loop logic slightly or add an explicit segment.

        // Alternative: If the goal is that "/" should match "/", the parsing needs to yield a comparable structure.
        // For now, let's focus on NOT returning empty for "/"
    }

    // handle trail slash (only if it's not the only character)
    if (normalized_path.length() > 1 && normalized_path.back() == '/') {
        normalized_path.pop_back();
    }

    // If after normalization, the path is empty (e.g. original path was ""), return empty.
    // Or if it was just "/", and we decided not to normalize it further.
    // Let's reconsider the loop based on the "/" case.

    std::stringstream ss(normalized_path);
    std::string segment_str;
    bool first_segment = true; // Flag to handle the leading slash correctly

    while (std::getline(ss, segment_str, '/')) {
        // The first segment might be empty if the path starts with '/'
        // If the original path was "/", getline might yield "" first.
        if (segment_str.empty()) {
            // If it's the root path "/", the first segment read will be empty.
            // If it's like "//path", the first empty segment should be skipped.
            // If it's "/path/", the last empty segment should be skipped if trailing slash removed.
            // The current `continue` is generally correct for skipping truly empty segments like in "//".
            // But for the root "/" case, we need to ensure something is represented.

            // If the original path was exactly "/", and this is the first (and only) segment read:
            if (path == "/" && first_segment) {
                 // We can either add an empty PathSegment here, or ensure the logic handles this.
                 // Let's try adding an empty segment to represent the root.
                 PathSegment segment;
                 segment.is_dynamic = false;
                 segment.value = ""; // Explicitly empty for root
                 path_segments.push_back(std::move(segment));
                 // No continue here, as we've handled the root segment.
            } else {
                // Skip empty segments that result from "//" or trailing slashes (if not handled above)
                continue;
            }
        } else {
            PathSegment segment;
            if (segment_str[0] == ':') {
                segment.is_dynamic = true;
                segment.value = segment_str.substr(1); // remove ':'
                if (segment.value.empty()) {
                    // illegal thing
                    backendLog("illegal format used: " + path, ERROR);
                    continue;
                }
            } else {
                segment.is_dynamic = false;
                segment.value = segment_str;
            }
            path_segments.push_back(std::move(segment));
        }
        first_segment = false; // after processing the first potential segment
    }

    if (path_segments.empty() && path == "/") {
        PathSegment segment;
        segment.is_dynamic = false;
        segment.value = ""; // represent root
        path_segments.push_back(std::move(segment));
    }

    return path_segments;
}

Route::Route(
        std::string name,
        std::string path,
        std::string handlerScript,
        const std::set<HTTPMethod>& methods_to_add,
        std::set<std::string> middlewares,
        const AccessLevel accessLevel
    ) :
        name(std::move(name)),
        original_path(std::move(path)),
        parsed_path(parse_path(original_path)),
        accessLevel(accessLevel),
        handlerScript(std::move(handlerScript)),
        middlewareScripts(std::move(middlewares))
{
    // insert all methodes
    methods.insert(methods_to_add.begin(), methods_to_add.end());
}

std::optional<std::unordered_map<std::string, std::string>> Route::match(
    const std::string &requestPath,
    HTTPMethod requestMethod
) const {

    // checks if this route supports requested method or not
    if (!methods.contains(requestMethod)) {
        return std::nullopt;
    }

    // checks if requested path is valid or not
    const auto request_segments = parse_path(requestPath);
    if (request_segments.empty()) {
        return std::nullopt;
    }

    // compares the length of requested path segments and parsed path
    if (request_segments.size() != parsed_path.size()) {
        return std::nullopt;
    }

    std::unordered_map<std::string, std::string> parameters;
    for (unsigned int i = 0; i < parsed_path.size(); i++) {
        const PathSegment &req_segment = request_segments[i];
        const PathSegment &parsed_segment = parsed_path[i];

        // if parsed segment is static so the value should be the same
        if (!parsed_segment.is_dynamic) {

            if (parsed_segment.value != req_segment.value) {
                return std::nullopt;
            }

        // else it's okay
        } else {
            parameters[parsed_segment.value] = req_segment.value;
        }
    }

    return parameters;
}

Router::Router() {
    try {
        auto routes_json = fs::readJson("routes.json");
        if (!routes_json.is_array()) {
            backendLog("could not read routes.json: json should be array", ERROR);
             exit(1);
        }

        for (auto& route_item : routes_json) {
            add_route_with_json(route_item);
        }

        bool not_found_exists = false;
        for(const auto& rt : static_routes) {
            if (rt->original_path == not_found_route_path) {
                not_found_route_ptr = rt.get();
                not_found_exists = true;
                break;
            }
        }
        if (!not_found_exists) {
            Route not_found_route(
                "page-not-found",
                not_found_route_path,
                "handle",
                {DELETE, POST, GET, PUT},
                {"middle"},
                ADMIN
            );
            add_route(std::move(not_found_route));
            for(const auto& rt : static_routes) {
                if (rt->original_path == not_found_route_path) {
                    not_found_route_ptr = rt.get();
                    break;
                }
            }
            if (!not_found_route_ptr) {
                backendLog("failed to set not_found_route_ptr!", ERROR);
            }
        }

    } catch (const std::exception &e) {
        backendLog("error while parsing routes.json: " + std::string(e.what()), ERROR);
    }
}



HTTPMethod string_to_http_method(const std::string& method_str) {
    if (method_str == "GET") return HTTPMethod::GET;
    if (method_str == "POST") return HTTPMethod::POST;
    if (method_str == "DELETE") return HTTPMethod::DELETE;
    if (method_str == "PUT") return HTTPMethod::PUT;
    return HTTPMethod::UNKNOWN;
}

AccessLevel string_to_access_level(const std::string& level_str) {
    if (level_str == "ADMIN") return AccessLevel::ADMIN;
    if (level_str == "USER") return AccessLevel::USER;
    if (level_str == "ANON") return AccessLevel::ANON;
    return AccessLevel::ANON;
}

Route Router::parse_route_json(nlohmann::json &json) {
    std::string name = json["name"];
    std::string path = json["path"];
    std::string handlerScript = json["handler"];

    std::vector<std::string> middlewareScripts;
    for (const auto& middleware : json["middlewares"]) {
        middlewareScripts.push_back(middleware);
    }

    AccessLevel accessLevel = ANON;
    if (json.contains("access")) {
        accessLevel = string_to_access_level(json["access"].get<std::string>());
    }

    std::set<HTTPMethod> methods_to_add;
    if (json.contains("methods") && json["methods"].is_array()) {
        for (const auto& method : json["methods"]) {
            methods_to_add.insert(string_to_http_method(method.get<std::string>()));
        }
    } else {
        backendLog("no method provided. using default (GET)", WARNING);
        methods_to_add.insert(string_to_http_method("GET"));
    }

    std::set<std::string> middlewares;
    if (json.contains("middlewares")) {
        for (const auto& middlewares : json["middlewares"]) {
            middlewareScripts.push_back(middlewares.get<std::string>());
        }
    }

    if (name.empty() || path.empty() || handlerScript.empty()) {
        backendLog("Skipping route with missing name, path, or handler in JSON.", ERROR);
        return {"invalid", "/invalid/", "", {}, {}, AccessLevel::ANON};
    }

    return {std::move(name), std::move(path), std::move(handlerScript),
                 methods_to_add, std::move(middlewares), accessLevel};
}

void Router::add_route_with_json(nlohmann::json &json) {
    try {
        Route route = Router::parse_route_json(json);
        if (route.name != "invalid") {
            add_route(std::move(route));
        }
    } catch (const std::exception &e) {
        backendLog("error adding route with json: " + std::string(e.what()), ERROR);
    }
}

void Router::add_route(Route route) {

    std::string name = route.name;

    if (routes_by_name.contains(route.name)) {
        backendLog("duplicated route with name: " + route.name, ERROR);
        return;
    }

    auto Route_ptr = std::make_unique<Route>(std::move(route));
    Route* raw_ptr = Route_ptr.get();

    routes_by_name[name] = raw_ptr;

    bool has_dynamic = false;
    for (const auto&[value, is_dynamic] : raw_ptr->parsed_path) {
        if (is_dynamic) {
            has_dynamic = true;
            break;
        }
    }

    if (has_dynamic) {
        dynamic_routes.push_back(std::move(Route_ptr));
    } else {
        static_routes.push_back(std::move(Route_ptr));
    }

    backendLog("adding route with name: " + name, WARNING);
    backendLog("count of static routes: " + std::to_string(static_routes.size()), WARNING);
    backendLog("count of dynamic routes: " + std::to_string(dynamic_routes.size()), WARNING);

    for (const auto& route : dynamic_routes) {
        std::string test;
        for (const auto& method : route->methods) {
            test += method;
        }
        backendLog(route->name + " " + std::to_string(route->accessLevel) + " " + test, PRINT);
    }

    for (const auto& route : static_routes) {
        std::string test;
        for (const auto& method : route->methods) {
            test += method;
        }
        backendLog(route->name + " " + std::to_string(route->accessLevel) + " " + test, PRINT);
    }
}

Route* Router::find_route(
    const std::string& requestPath,
    const HTTPMethod requestMethod,
    std::unordered_map<std::string, std::string>& out_params)
{
    backendLog("request path: '" + requestPath + "'", WARNING);
    for (const auto& rt : static_routes) {

        backendLog("STATIC routes: " + rt->name + " path: '" + rt->original_path + "'", WARNING);

        // if the requested method supported by the route then
        if (rt->methods.contains(requestMethod)) {
            backendLog("contains 332", PRINT);
            // if requested segment matches parsed segment then we have a key value map
            auto params_opt = rt->match(requestPath, requestMethod);
            if (params_opt) {
                // now we have a ready to send for lua route
                out_params = std::move(params_opt.value());
                return rt.get();
            }
        }
    }

    for (const auto& rt : dynamic_routes) {

        backendLog("DYNAMIC routes: " + rt->name + " path: '" + rt->original_path + "'", WARNING);

        // if the requested method supported by the route then
         if (rt->methods.contains(requestMethod)) {
             backendLog("contains dynamic 346", PRINT);
             // if requested segment matches parsed segment then we have a key value map
            auto params_opt = rt->match(requestPath, requestMethod);
            if (params_opt) {
                // now we have a ready to send for lua route
                out_params = std::move(params_opt.value());
                return rt.get();
            }
        }
    }

    if (not_found_route_ptr) {
        auto params_opt = not_found_route_ptr->match(requestPath, requestMethod);
        if (params_opt) {
             out_params = std::move(params_opt.value());
             return not_found_route_ptr;
        } else {
             out_params.clear();
             return not_found_route_ptr;
        }
    }

    backendLog("no routes found, including the Not Found route!", ERROR);
    return nullptr;
}