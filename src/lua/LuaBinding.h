#pragma once
#include <sol/sol.hpp>
#include "LuaObject.h"

extern sol::state lua;
extern std::vector<LuaObject> scripts;

class Lua {
public:

    static Lua& getInstance() {
        static Lua instance;
        return instance;
    }

    void Initialize();
    void LoadScripts();

private:
    sol::state lua;
};