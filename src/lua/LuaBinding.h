#pragma once
#include <sol/sol.hpp>
#include "LuaObject.h"

extern sol::state lua;
extern std::vector<LuaObject> scripts;

void BindToLua(sol::state& lua);

class Lua {
public:

    static Lua& getInstance() {
        static Lua instance;
        return instance;
    }

    void Initialize();
    void LoadScripts();

    static sol::function GetFunction(const std::string& name);

private:
    sol::state lua;
};

std::vector<std::string> split(const std::string& s, char delimiter);