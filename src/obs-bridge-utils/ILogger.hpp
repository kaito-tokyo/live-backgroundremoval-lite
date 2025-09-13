/*
obs-bridge-utils
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace kaito_tokyo {
namespace obs_bridge_utils {

class ILogger {
public:
	virtual ~ILogger() noexcept = default;

    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
        format_and_log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
        format_and_log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
        format_and_log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) const noexcept {
        format_and_log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

protected:
    enum class LogLevel : std::int8_t {
        Debug,
        Info,
        Warn,
        Error
    };

    virtual void log(LogLevel level, const std::string& message) const noexcept = 0;
    virtual LogLevel get_current_level() const noexcept {
        return LogLevel::Debug;
    }

private:
    template<typename... Args>
    void format_and_log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) const noexcept try {
        if (level >= get_current_level()) {
            const std::string message = fmt::format(fmt, std::forward<Args>(args)...);
            log(level, message);
        }
    } catch (const std::exception& e) {
        std::fprintf(std::stderr, "[LOGGER FATAL] Failed to format log message: %s\n", e.what());
    } catch (...) {
        std::fprintf(std::stderr, "[LOGGER FATAL] An unknown error occurred while formatting log message.\n");
    }
};

} // namespace obs_bridge_utils
} // namespace kaito_tokyo
