/*
 * Logger
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
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
