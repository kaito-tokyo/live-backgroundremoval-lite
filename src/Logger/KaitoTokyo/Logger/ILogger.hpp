/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo Logger Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
