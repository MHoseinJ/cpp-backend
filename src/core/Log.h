#pragma once


enum TYPE {
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    PRINT,
};

void backendLog(const std::string& message, TYPE type);