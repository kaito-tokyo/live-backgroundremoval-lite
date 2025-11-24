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

#include <mutex>
#include <string_view>
#include <string>

#include <util/base.h>

#include "ILogger.hpp"

namespace KaitoTokyo {
namespace Logger {

class NullLogger final : public ILogger {
public:
	NullLogger() = default;
	~NullLogger() noexcept override = default;
	bool isInvalid() const noexcept override { return true; }

protected:
	void log(LogLevel, std::string_view) const noexcept override
	{
		// No-op
	}

	const char *getPrefix() const noexcept override { return nullptr; }
};

} // namespace Logger
} // namespace KaitoTokyo
