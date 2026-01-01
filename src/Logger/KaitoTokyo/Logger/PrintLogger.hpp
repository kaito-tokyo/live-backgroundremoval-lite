/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo Logger Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
