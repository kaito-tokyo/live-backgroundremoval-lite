// SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <source_location>
#include <span>
#include <string_view>

#include "ILogger.hpp"

namespace KaitoTokyo::Logger {

class NullLogger : public ILogger {
public:
	NullLogger() = default;

	~NullLogger() override = default;

	void log(LogLevel, std::string_view, std::source_location, std::span<const LogField>) const noexcept override
	{
		// No-op
	}

	static std::shared_ptr<NullLogger> instance()
	{
		static std::shared_ptr<NullLogger> instance = std::make_shared<NullLogger>();
		return instance;
	}
};

} // namespace KaitoTokyo::Logger
