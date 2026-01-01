/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * KaitoTokyo ObsBridgeUtils Library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

			fmt::format_to(std::back_inserter(buffer), "{} name={}\tlocation={}:{}", prefix_, name,
				       loc.function_name(), loc.line());
			for (const Logger::LogField &field : context) {
				fmt::format_to(std::back_inserter(buffer), "\t{}={}", field.key, field.value);
			}

			blog(blogLevel, "%.*s", static_cast<int>(buffer.size()), buffer.data());
		} catch (...) {
			blog(blogLevel, "%s name=LoggerPanic\tlocation=%s:%d", prefix_.c_str(), loc.function_name(),
			     loc.line());
		}
	}

private:
	const std::string prefix_;
};

} // namespace ObsBridgeUtils
} // namespace KaitoTokyo
