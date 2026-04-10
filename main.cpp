#include "src/core/Core.h"
#include "src/routing/Router.h"

int main() {

    Router router;

    HTTPServer server;
    server.Loop();



    return 0;
}
