#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

inline static std::string getRuntimePath()
{
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return (std::filesystem::path(home) / ".local" / "state" / "usic").string();
}

inline const std::string RUNTIME_PATH = getRuntimePath();
