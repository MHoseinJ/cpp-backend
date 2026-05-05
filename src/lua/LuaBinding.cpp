#include "LuaBinding.h"
#include <sol/sol.hpp>

#include "../core/HTTPObject.h"
#include "../core/Log.h"
#include "../template/Template.h"
#include "../utils/FileSystem.h"

sol::state lua;
std::vector<LuaObject> scripts;

void Lua::Initialize() {
    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::io,
        sol::lib::table
    );

    BindToLua(lua);
}

void Lua::LoadScripts() {

    const auto ScriptNames = fs::listFiles("scripts");
    for (const auto& Name : ScriptNames) {
        sol::environment env(lua, sol::create, lua.globals());

        auto chunk = lua.load_file(Name);
        if (!chunk.valid()) {
            backendLog("LUA load error: " + Name, ERROR);
            continue;
        }

        sol::protected_function func = chunk;
        auto result = func(env);

        if (!result.valid()) {
            backendLog("LUA runtime error: " + Name, ERROR);
            continue;
        }

        const auto name = env["MY_NAME_IS"];

        LuaObject obj;
        obj.name = name;
        obj.env = std::move(env);

        scripts.push_back(std::move(obj));
    }
}

sol::function Lua::GetFunction(const std::string &name) {
    const auto key_value = split(name, '.');
    if (key_value.size() != 2) {
        backendLog("invalid function name '" + name + "'", ERROR);
        return nullptr;
    }
    const auto& name_string = key_value[0];
    auto value = key_value[1];

    backendLog("trying to find...", WARNING);

    for (auto &script : scripts) {

        if (script.name == name_string) {
            if (script.env[value]) {
                backendLog("found script: " + script.name, WARNING);
                return script.env[value];
            }
        }
    }

    backendLog("No such script: " + name, ERROR);
    return nullptr;
}

std::vector<std::string> split(const std::string &s, const char delimiter) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        elems.push_back(item);
    }
    return elems;
}

void BindToLua(sol::state &lua) {
    lua.new_enum<HTTPMethod>("HTTPMethod", {
        {"GET", GET},
        {"POST", POST},
        {"DELETE", DELETE},
        {"PUT", PUT},
        {"UNKNOWN", UNKNOWN}
    });

    lua.new_usertype<HTTPRequest>(
        "HTTPRequest",
        "method", &HTTPRequest::method,
        "raw_url", &HTTPRequest::raw_url,
        "body", &HTTPRequest::body,
        "path_parameters", &HTTPRequest::get_path_parameters,
        "query_parameters" , &HTTPRequest::get_query_parameters
    );

    lua.new_usertype<nlohmann::json>(
        "JSON",
        "dump", &nlohmann::json::dump
    );

    lua["template"] = lua.create_table();

    lua["template"]["render"] = [](const njson& tmpl, const njson& data) {
        return TemplateEngine::getEngine().render_WITH_JSON_TMPL(tmpl, data);
    };

    lua["template"]["renderJson"] = [](const std::string& tmpl, const njson& data) {
        return TemplateEngine::getEngine().renderToJson(tmpl, data);
    };

    lua["template"]["get"] = [&lua](const njson& data, const std::string& path) -> sol::object {
        const auto value = TemplateEngine::getNestedJson(data, path);
        if (!value.has_value()) {
            return sol::nil;
        }

        const njson& v = value.value();
        if (v.is_string()) return sol::make_object(lua, v.get<std::string>());
        if (v.is_number()) return sol::make_object(lua, v.get<double>());
        if (v.is_boolean()) return sol::make_object(lua, v.get<bool>());
        if (v.is_null()) return sol::nil;

        return sol::make_object(lua, v);
    };

    lua["fs"] = lua.create_table();

    lua["fs"]["loadJson"] = &fs::readJson;
}