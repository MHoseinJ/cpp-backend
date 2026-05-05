#pragma once
#include <optional>
#include "../utils/json.hpp"

using njson = nlohmann::json;
using namespace std;

class TemplateEngine {
public:
    static TemplateEngine& getEngine();

    // helpers
    static optional<njson> getNestedJson(const njson& json, const string& path);

    static string jsonToString(const njson& json);

    static njson processNode(const njson& node, const njson& data);

    static string render(const string& jTemplate, const njson& data);
    static njson renderToJson(const string& jTemplate, const njson& data);
    static string render_WITH_JSON_TMPL(const njson& jTemplate, const njson& data);
};