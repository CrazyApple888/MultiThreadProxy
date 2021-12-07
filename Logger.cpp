#include "Logger.h"

Logger::Logger(bool isDebug, std::ostream &ostream) : is_debug(isDebug), ostream(ostream) {}

void Logger::info(const std::string& tag, const std::string& message) {
    ostream << "[INFO] " + tag + ": " + message << std::endl;
}

void Logger::debug(const std::string &tag, const std::string &message) {
    if (is_debug) {
        ostream << "[DEBUG] " + tag + ": " + message << std::endl;
    }
}

bool Logger::isDebug() const {
    return is_debug;
}