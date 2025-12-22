/*
 * KaitoTokyo Logger Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace KaitoTokyo::Logger {

/**
 * @class ILogger
 * @brief A thread-safe, noexcept interface for polymorphic logging.
 *
 * Provides a robust, exception-safe logging contract. All public logging
 * methods (debug, info, warn, error) are guaranteed not to throw exceptions.
 * If an internal formatting error occurs (e.g., std::bad_alloc),
 * the logger will fall back to writing a fatal error to `stderr`.
 *
 * This interface is designed to be implemented by concrete logger classes
 * and is non-copyable and non-movable.
 */
class ILogger {
public:
	/**
	 * @brief Default constructor.
	 */
	ILogger() noexcept = default;

	/**
	 * @brief Virtual destructor to ensure correct cleanup of derived classes.
	 */
	virtual ~ILogger() noexcept = default;

	/** @name Non-Copyable and Non-Movable
	 * @{
	 */
	ILogger(const ILogger &) = delete;
	ILogger &operator=(const ILogger &) = delete;
	ILogger(ILogger &&) = delete;
	ILogger &operator=(ILogger &&) = delete;
	/** @} */

	/**
	 * @brief Logs a debug message using fmt-style formatting.
	 * Guaranteed not to throw an exception.
	 *
	 * @tparam Args Parameter pack for format arguments.
	 * @param fmt The fmt-style format string.
	 * @param args The arguments to format.
	 */
	template<typename... Args> void debug(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Debug, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Logs an info message using fmt-style formatting.
	 * Guaranteed not to throw an exception.
	 *
	 * @tparam Args Parameter pack for format arguments.
	 * @param fmt The fmt-style format string.
	 * @param args The arguments to format.
	 */
	template<typename... Args> void info(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Info, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Logs a warning message using fmt-style formatting.
	 * Guaranteed not to throw an exception.
	 *
	 * @tparam Args Parameter pack for format arguments.
	 * @param fmt The fmt-style format string.
	 * @param args The arguments to format.
	 */
	template<typename... Args> void warn(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Warn, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Logs an error message using fmt-style formatting.
	 * Guaranteed not to throw an exception.
	 *
	 * @tparam Args Parameter pack for format arguments.
	 * @param fmt The fmt-style format string.
	 * @param args The arguments to format.
	 */
	template<typename... Args> void error(fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	{
		formatAndLog(LogLevel::Error, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Logs an exception with context.
	 *
	 * Guaranteed not to throw an exception. This method is safe to call
	 * from within a catch block.
	 *
	 * @param e The exception that was caught.
	 * @param context A string view describing the context where the exception occurred.
	 */
	void logException(const std::exception &e, std::string_view context) const noexcept
	{
		error("{}: {}\n", context, e.what());
	}

	/**
	 * @brief Returns true if this logger is an invalid logger.
	 */
	virtual bool isInvalid() const noexcept { return false; }

protected:
	enum class LogLevel : std::int8_t { Debug, Info, Warn, Error };

	virtual void log(LogLevel level, std::string_view message) const noexcept = 0;
	virtual const char *getPrefix() const noexcept = 0;

private:
	template<typename... Args>
	void formatAndLog(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	try {
		fmt::memory_buffer buffer;
		fmt::format_to(std::back_inserter(buffer), "{} ", getPrefix());
		fmt::vformat_to(std::back_inserter(buffer), fmt, fmt::make_format_args(args...));

		log(level, {buffer.data(), buffer.size()});
	} catch (const std::exception &e) {
		fprintf(stderr, "[%s] [LOGGER FATAL] Failed to format log message: %s\n", getPrefix(), e.what());
	} catch (...) {
		fprintf(stderr, "[%s] [LOGGER FATAL] An unknown error occurred while formatting log message.\n",
			getPrefix());
	}
};

} // namespace KaitoTokyo::Logger
