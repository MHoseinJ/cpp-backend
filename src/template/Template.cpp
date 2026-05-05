#include "Template.h"
#include <regex>
#include "../core/Log.h"

TemplateEngine& TemplateEngine::getEngine() {
    static TemplateEngine engine;
    return engine;
}

optional<njson> TemplateEngine::getNestedJson(const njson& json, const string& path) {
    if (path.empty()) {
        return std::nullopt;
    }

    njson current_data = json;
    std::stringstream ss(path);
    std::string segment;

    while (std::getline(ss, segment, '.')) {

        while (!segment.empty() && segment[0] == ' ') segment.erase(0, 1);
        while (!segment.empty() && segment.back() == ' ') segment.pop_back();

        if (!current_data.is_object() || !current_data.contains(segment)) {
            return std::nullopt;
        }
        current_data = current_data[segment];
    }
    return current_data;
}

string TemplateEngine::jsonToString(const njson& json) {
    if (json.is_string()) {
        return json.get<string>();
    }
    if (json.is_number_integer()) {
        return to_string(json.get<int64_t>());
    }
    if (json.is_number_float()) {
        return to_string(json.get<double>());
    }
    if (json.is_boolean()) {
        return json.get<bool>() ? "true" : "false";
    }
    if (json.is_null()) {
        return "null";
    }
    return json.dump(2);
}

njson TemplateEngine::processNode(const njson& node, const njson& data) {
    if (node.is_object()) {
        njson result = njson::object();
        for (const auto& [key, value] : node.items()) {
            result[key] = processNode(value, data);
        }
        return result;
    }

    if (node.is_array()) {
        njson result = njson::array();
        for (const auto& item : node) {
            result.push_back(processNode(item, data));
        }
        return result;
    }

    if (node.is_string()) {
        std::string str = node.get<std::string>();
        std::regex pattern(R"(\{\{\s*([^{}]+?)\s*\}\})");
        std::smatch match;
        std::string result = str;

        auto it = std::sregex_iterator(str.begin(), str.end(), pattern);
        while (it != std::sregex_iterator()) {
            match = *it;
            std::string fullMatch = match[0].str();
            std::string path = match[1].str();

            if (auto valueOpt = getNestedJson(data, path); valueOpt.has_value()) {
                std::string replacement = jsonToString(valueOpt.value());
                size_t pos = result.find(fullMatch);
                if (pos != std::string::npos) {
                    result.replace(pos, fullMatch.length(), replacement);
                }
            }
            ++it;
        }
        return {result};
    }

    return node;
}

string TemplateEngine::render(const string& jTemplate, const njson& data) {
    try {
        const njson template_json = njson::parse(jTemplate);
        const njson result = processNode(template_json, data);
        return result.dump(2);
    } catch (const njson::parse_error& e) {
        backendLog(e.what(), ERROR);
        return R"({"error": "JSON parse error", "details": ")" + std::string(e.what()) + "\"}";
    } catch (const std::exception& e) {
        backendLog(e.what(), ERROR);
        return R"({"error": ")" + std::string(e.what()) + "\"}";
    }
}

string TemplateEngine::render_WITH_JSON_TMPL(const njson& jTemplate, const njson& data) {
    try {
        const njson result = processNode(jTemplate, data);
        return result.dump(2);
    } catch (const njson::parse_error& e) {
        backendLog(e.what(), ERROR);
        return R"({"error": "JSON parse error", "details": ")" + std::string(e.what()) + "\"}";
    } catch (const std::exception& e) {
        backendLog(e.what(), ERROR);
        return R"({"error": ")" + std::string(e.what()) + "\"}";
    }
}

njson TemplateEngine::renderToJson(const string& jTemplate, const njson& data) {
    const njson template_json = njson::parse(jTemplate);
    return processNode(template_json, data);
}