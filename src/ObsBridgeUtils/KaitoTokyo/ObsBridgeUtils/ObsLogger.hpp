// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <string_view>
#include <string>

#include <fmt/format.h>

#include <util/base.h>

#include <KaitoTokyo/Logger/ILogger.hpp>

namespace KaitoTokyo {
namespace ObsBridgeUtils {

class ObsLogger final : public Logger::ILogger {
public:
	ObsLogger(std::string prefix) : prefix_(std::move(prefix)) {}

	~ObsLogger() override = default;

protected:
	void log(Logger::LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const Logger::LogField> context) const noexcept override
	{
		int blogLevel;
		switch (level) {
		case Logger::LogLevel::Debug:
			blogLevel = LOG_DEBUG;
			break;
		case Logger::LogLevel::Info:
			blogLevel = LOG_INFO;
			break;
		case Logger::LogLevel::Warn:
			blogLevel = LOG_WARNING;
			break;
		case Logger::LogLevel::Error:
			blogLevel = LOG_ERROR;
			break;
		default:
			blog(LOG_ERROR, "%s name=UnknownLogLevelError\tlocation=%s:%d\n", prefix_.c_str(),
			     loc.function_name(), loc.line());
			return;
		}

		try {
			fmt::basic_memory_buffer<char, 4096> buffer;

			fmt::format_to(std::back_inserter(buffer), "{} name={}", prefix_, name);
			for (const Logger::LogField &field : context) {
				fmt::format_to(std::back_inserter(buffer), "\t{}={}", field.key, field.value);
			}

			blog(blogLevel, "%.*s", static_cast<int>(buffer.size()), buffer.data());
		} catch (...) {
			blog(blogLevel, "%s name=LoggerPanic", prefix_.c_str());
		}
	}

private:
	const std::string prefix_;
};

} // namespace ObsBridgeUtils
} // namespace KaitoTokyo
