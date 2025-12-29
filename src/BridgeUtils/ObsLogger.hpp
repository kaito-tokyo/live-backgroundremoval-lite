/*
 * BridgeUtils
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#pragma once

#include <source_location>
#include <span>
#include <string_view>

#include <fmt/format.h>

#include <util/base.h>

#include <ILogger.hpp>

namespace KaitoTokyo::BridgeUtils {

class ObsLogger : public Logger::ILogger {
public:
	ObsLogger(std::string_view prefix) noexcept : prefix_(prefix) {}

	~ObsLogger() override = default;

protected:
	void log(LogLevel level, std::string_view message) const noexcept override
	{
		int blogLevel = getBlogLevel(level);
		if (blogLevel < 0) {
			blog(LOG_ERROR, "%.*s Unknown log level: %d", static_cast<int>(prefix_.size()), prefix_.data(),
			     static_cast<int>(level));
			return;
		}

		blog(blogLevel, "%.*s %.*s", static_cast<int>(prefix_.size()), prefix_.data(),
		     static_cast<int>(message.length()), message.data());
	}

	void log(LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const Logger::LogField> context) const noexcept override
	{
		int blogLevel = getBlogLevel(level);
		if (blogLevel < 0) {
			std::source_location errloc = std::source_location::current();
			blog(LOG_ERROR, "%.*s name=UnknownLogLevelError\tlocation=%s:%d\n",
			     static_cast<int>(prefix_.size()), prefix_.data(), errloc.file_name(), errloc.line());
			return;
		}

		try {
			fmt::basic_memory_buffer<char, 4096> buffer;

			fmt::format_to(std::back_inserter(buffer), "{} name={}\tlocation={}:{}", prefix_, name,
				       loc.file_name(), loc.line());
			for (const Logger::LogField &field : context) {
				fmt::format_to(std::back_inserter(buffer), "\t{}={}", field.key, field.value);
			}

			blog(blogLevel, "%.*s", static_cast<int>(buffer.size()), buffer.data());
		} catch (...) {
			std::source_location errloc = std::source_location::current();
			blog(blogLevel, "%.*s name=LoggerPanic\tlocation=%s:%d", static_cast<int>(prefix_.size()),
			     prefix_.data(), errloc.file_name(), errloc.line());
		}
	}

private:
	const std::string_view prefix_;

	static int getBlogLevel(LogLevel level) noexcept
	{
		switch (level) {
		case LogLevel::Debug:
			return LOG_DEBUG;
		case LogLevel::Info:
			return LOG_INFO;
		case LogLevel::Warn:
			return LOG_WARNING;
		case LogLevel::Error:
			return LOG_ERROR;
		default:
			return -1;
		}
	}
};

} // namespace KaitoTokyo::BridgeUtils
