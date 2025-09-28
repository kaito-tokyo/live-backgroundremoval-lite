/*
Bridge Utils
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
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace KaitoTokyo {
namespace BridgeUtils {

class ILogger {
public:
	ILogger() noexcept = default;
	virtual ~ILogger() noexcept = default;

	ILogger(const ILogger &) = delete;
	ILogger &operator=(const ILogger &) = delete;
	ILogger(ILogger &&) = delete;
	ILogger &operator=(ILogger &&) = delete;

	template<typename... Args> void debug(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Debug, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args> void info(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Info, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args> void warn(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Warn, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args> void error(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Error, fmt, std::forward<Args>(args)...);
	}

	virtual void logException(const std::exception &e, std::string_view context) const noexcept = 0;

protected:
	enum class LogLevel : std::int8_t { Debug, Info, Warn, Error };

	virtual void log(LogLevel level, std::string_view message) const noexcept = 0;

	virtual std::string_view getPrefix() const noexcept { return ""; }

private:
	template<typename... Args>
	void formatAndLog(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		try {
			fmt::memory_buffer buffer;
			fmt::format_to(std::back_inserter(buffer), "{}", getPrefix());
			fmt::vformat_to(std::back_inserter(buffer), fmt, fmt::make_format_args(args...));
			log(level, {buffer.data(), buffer.size()});
		} catch (const std::exception &e) {
			fprintf(stderr, "[LOGGER FATAL] Failed to format log message: %s\n", e.what());
		} catch (...) {
			fprintf(stderr, "[LOGGER FATAL] An unknown error occurred while formatting log message.\n");
		}
	}
};

} // namespace BridgeUtils
} // namespace KaitoTokyo
