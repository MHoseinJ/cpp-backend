#include "LuaBinding.h"
#include <sol/sol.hpp>
#include "../core/Log.h"
#include "../utils/FileSystem.h"

std::vector<LuaObject> scripts;

void Lua::Initialize() {
    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::io,
        sol::lib::table
    );
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
