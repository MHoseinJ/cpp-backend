#pragma once
#include <sol/sol.hpp>

struct LuaObject {
    std::string name;
    sol::environment env;
    std::vector<sol::function*> functions;
};