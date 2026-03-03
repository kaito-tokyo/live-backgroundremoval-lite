// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iterator>
#include <source_location>
#include <span>
#include <string_view>

namespace KaitoTokyo::Logger {

struct LogField {
	std::string_view key;
	std::string_view value;
};

enum class LogLevel { Debug, Info, Warn, Error };

class ILogger {
public:
	ILogger() = default;

	virtual ~ILogger() noexcept = default;

	ILogger(const ILogger &) = delete;
	ILogger &operator=(const ILogger &) = delete;
	ILogger(ILogger &&) = delete;
	ILogger &operator=(ILogger &&) = delete;

	void debug(std::string_view name, std::initializer_list<LogField> context = {},
		   std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Debug, name, loc, context);
	}

	void info(std::string_view name, std::initializer_list<LogField> context = {},
		  std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Info, name, loc, context);
	}

	void warn(std::string_view name, std::initializer_list<LogField> context = {},
		  std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Warn, name, loc, context);
	}

	void error(std::string_view name, std::initializer_list<LogField> context = {},
		   std::source_location loc = std::source_location::current()) const noexcept
	{
		log(LogLevel::Error, name, loc, context);
	}

	virtual void log(LogLevel level, std::string_view name, std::source_location loc,
			 std::span<const LogField> context) const noexcept = 0;
};

} // namespace KaitoTokyo::Logger
