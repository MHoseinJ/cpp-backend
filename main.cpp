#include "src/core/Core.h"
#include "src/routing/Router.h"

int main() {

    Router router;
    std::unordered_map<std::string, std::string> test;
    test["test"] = "test";
    router.find_route("user_profile", GET, test);
    HTTPServer server;
    server.Loop();



    return 0;
}
