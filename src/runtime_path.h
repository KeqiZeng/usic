#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

/**
 * Gets the runtime path of the program.
 *
 * The runtime path is where the program stores its log files and named pipes. It is
 * determined by the HOME environment variable, and is of the form
 * $HOME/.local/state/usic.
 *
 * If the HOME environment variable is not set, a std::runtime_error is
 * thrown.
 *
 * @return The runtime path of the program as a string.
 */
inline static std::string getRuntimePath()
{
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return (std::filesystem::path(home) / ".local" / "state" / "usic").string();
}

inline const std::string RUNTIME_PATH = getRuntimePath();
