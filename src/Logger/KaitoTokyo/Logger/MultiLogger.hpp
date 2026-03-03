// SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: MIT

#pragma once

#include <iterator>
#include <source_location>
#include <span>
#include <string_view>
#include <vector>
#include <memory>

#include "ILogger.hpp"

namespace KaitoTokyo::Logger {

class MultiLogger : public ILogger {
public:
	explicit MultiLogger(std::vector<std::shared_ptr<const ILogger>> loggers) : loggers_(std::move(loggers)) {}

	virtual ~MultiLogger() noexcept = default;

	MultiLogger(const MultiLogger &) = delete;
	MultiLogger &operator=(const MultiLogger &) = delete;
	MultiLogger(MultiLogger &&) = delete;
	MultiLogger &operator=(MultiLogger &&) = delete;

	void log(LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const LogField> context) const noexcept override
	{
		for (const std::shared_ptr<const ILogger> &logger : loggers_) {
			logger->log(level, name, loc, context);
		}
	}

private:
	std::vector<std::shared_ptr<const ILogger>> loggers_;
};

} // namespace KaitoTokyo::Logger
