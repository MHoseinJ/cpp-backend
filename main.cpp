#include <iostream>

#include "src/core/Core.h"
#include "src/core/Log.h"
#include "src/lua/LuaBinding.h"
#include "src/routing/Router.h"

int main() {

    Router router;

    Lua::getInstance().Initialize();
    Lua::getInstance().LoadScripts();

    const HTTPServer server;
    server.Loop(&router);

    return 0;
}
