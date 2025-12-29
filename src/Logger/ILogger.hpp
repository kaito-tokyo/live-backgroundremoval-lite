/*
 * KaitoTokyo Logger Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <exception>
#include <iterator>
#include <source_location>
#include <span>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace KaitoTokyo::Logger {

struct LogField {
	std::string_view key;
	std::string_view value;
};

class ILogger {
public:
	ILogger() noexcept = default;
	virtual ~ILogger() = default;

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

	void debug(std::string_view name, std::initializer_list<LogField> context,
		   std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Debug, name, loc, context);
	}

	void info(std::string_view name, std::initializer_list<LogField> context,
		  std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Info, name, loc, context);
	}

	void warn(std::string_view name, std::initializer_list<LogField> context,
		  std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Warn, name, loc, context);
	}

	void error(std::string_view name, std::initializer_list<LogField> context,
		   std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Error, name, loc, context);
	}

	void logException(const std::exception &e, std::string_view context) const noexcept
	{
		error("{}: {}\n", context, e.what());
	}
	virtual bool isInvalid() const noexcept { return false; }

protected:
	enum class LogLevel { Debug, Info, Warn, Error };

	virtual void log(LogLevel level, std::string_view message) const noexcept = 0;
	virtual void log(LogLevel level, std::string_view name, std::source_location loc,
			 std::span<const LogField> context) const noexcept = 0;

private:
	template<typename... Args>
	void formatAndLog(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	try {
		fmt::basic_memory_buffer<char, 4096> buffer;
		fmt::vformat_to(std::back_inserter(buffer), fmt, fmt::make_format_args(args...));
		log(level, {buffer.data(), buffer.size()});
	} catch (const fmt::format_error &e) {
		log(LogLevel::Error, e.what());
	} catch (const std::exception &e) {
		log(LogLevel::Error, e.what());
	} catch (...) {
		log(LogLevel::Error, "LOGGER PANIC OCCURRED");
	}
};

} // namespace KaitoTokyo::Logger
