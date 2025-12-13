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

#include <mutex>
#include <string_view>
#include <string>

#include <util/base.h>

#include <ILogger.hpp>

namespace KaitoTokyo {
namespace BridgeUtils {

class ObsLogger final : public Logger::ILogger {
public:
	ObsLogger(const char *prefix) : prefix_(prefix ? prefix : "") {}
	~ObsLogger() noexcept override = default;

protected:
	static constexpr size_t MAX_LOG_CHUNK_SIZE = 4000;

	void log(LogLevel level, std::string_view message) const noexcept override
	{
		int blogLevel;
		switch (level) {
		case LogLevel::Debug:
			blogLevel = LOG_DEBUG;
			break;
		case LogLevel::Info:
			blogLevel = LOG_INFO;
			break;
		case LogLevel::Warn:
			blogLevel = LOG_WARNING;
			break;
		case LogLevel::Error:
			blogLevel = LOG_ERROR;
			break;
		default:
			std::lock_guard<std::mutex> lock(mutex_);
			blog(LOG_ERROR, "[LOGGER FATAL] Unknown log level: %d\n", static_cast<int>(level));
			return;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		if (message.length() <= MAX_LOG_CHUNK_SIZE) {
			blog(blogLevel, "%.*s", static_cast<int>(message.length()), message.data());
		} else {
			// Log in chunks if message is too long
			for (size_t i = 0; i < message.length(); i += MAX_LOG_CHUNK_SIZE) {
				const auto chunk = message.substr(i, MAX_LOG_CHUNK_SIZE);
				blog(blogLevel, "%.*s", static_cast<int>(chunk.length()), chunk.data());
			}
		}
	}

	const char *getPrefix() const noexcept override { return prefix_.c_str(); }

private:
	const std::string prefix_;
	mutable std::mutex mutex_;
};

} // namespace BridgeUtils
} // namespace KaitoTokyo
