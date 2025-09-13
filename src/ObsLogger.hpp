/*
OBS BackgroundRemoval Lite
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

#include <cstdarg>

#include "obs-bridge-utils/Logger.hpp"

#include "plugin-support.h"
#include <util/base.h>

class ObsLogger : public kaito_tokyo::obs_bridge_utils::ILogger {
public:
	void debug(const char *message, ...) const noexcept override {
		std::va_list args;
		va_start(args, message);
		obs_log(LOG_DEBUG, message, args);
		va_end(args);
	}
	void info(const char *message, ...) const noexcept override {
		std::va_list args;
		va_start(args, message);
		obs_log(LOG_INFO, message, args);
		va_end(args);
	}
	void warn(const char *message, ...) const noexcept override {
		std::va_list args;
		va_start(args, message);
		obs_log(LOG_WARNING, message, args);
		va_end(args);
	}
	void error(const char *message, ...) const noexcept override {
		std::va_list args;
		va_start(args, message);
		obs_log(LOG_ERROR, message, args);
		va_end(args);
	}
};
