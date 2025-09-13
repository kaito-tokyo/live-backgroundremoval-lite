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

#include <util/base.h>

#include "obs-bridge-utils/ILogger.hpp"

namespace kaito_tokyo {
namespace obs_bridge_utils {

class ObsLogger final : public ILogger {
public:
    ObsLogger(const std::string &pluginName) : prefix(pluginName + " ") {}

protected:
    virtual void log(LogLevel level, std::string_view message) const noexcept {
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
			fprintf(stderr, "[LOGGER FATAL] Unknown log level: %d\n", static_cast<int>(level));
			return;
		}

 		blog(blogLevel, "%.*s", static_cast<int>(message.length()), message.data());
    }

protected:
	virtual std::string_view getPrefix() const noexcept {
		return prefix;
	}

private:
	const std::string prefix;
};

} // namespace obs_bridge_utils
} // namespace kaito_tokyo

