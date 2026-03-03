// SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: MIT

#pragma once

#include <iostream>
#include <memory>
#include <source_location>
#include <span>
#include <string_view>

#include "ILogger.hpp"

namespace KaitoTokyo::Logger {

class PrintLogger : public ILogger {
public:
	PrintLogger() = default;

	~PrintLogger() override = default;

	void log(LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const LogField> context) const noexcept override
	{
		switch (level) {
		case LogLevel::Debug:
			std::cout << "level=DEBUG";
			break;
		case LogLevel::Info:
			std::cout << "level=INFO";
			break;
		case LogLevel::Warn:
			std::cout << "level=WARN";
			break;
		case LogLevel::Error:
			std::cout << "level=ERROR";
			break;
		default:
			std::cout << "level=UNKNOWN";
			break;
		}

		std::cout << "\tname=" << name << "\tlocation=" << loc.file_name() << ":" << loc.line();
		for (const auto &field : context) {
			std::cout << "\t" << field.key << "=" << field.value;
		}
		std::cout << std::endl;
	}

	static std::shared_ptr<PrintLogger> instance()
	{
		static std::shared_ptr<PrintLogger> instance = std::make_shared<PrintLogger>();
		return instance;
	}
};

} // namespace KaitoTokyo::Logger
