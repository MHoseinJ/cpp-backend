#include <iostream>

#include "Log.h"

const char* getTerminalColor(const TYPE type) {
    switch (type) {
        case ERROR:   return "\033[1;31m";
        case WARNING: return "\033[1;33m";
        case INFO:    return "\033[1;34m";
        case DEBUG:   return "\033[1;36m";
        default:      return "\033[0m";
    }
}

void backendLog(const std::string& message, TYPE type) {
    std::string prefix;
    switch (type) {
        case ERROR:   prefix = "[ERROR] "; break;
        case WARNING: prefix = "[WARNING] "; break;
        case INFO:    prefix = "[INFO] "; break;
        case DEBUG:   prefix = "[DEBUG] "; break;
        case PRINT:   prefix = ""; break;
        default:      prefix = "[UNKNOWN] "; break;
    }
    std::string combinedMsg = prefix + message;
    std::cout << getTerminalColor(type) << combinedMsg << "\033[0m" << std::endl;
}