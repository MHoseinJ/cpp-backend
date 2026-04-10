#include <iostream>

#include "src/core/Core.h"
#include "src/core/Log.h"
#include "src/routing/Router.h"

int main() {

    Router router;

    const HTTPServer server;
    server.Loop();

    return 0;
}
