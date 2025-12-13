/*
 * Logger
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#ifdef HAVE_BACKWARD
#include <sstream>
#include <backward.hpp>
#endif // HAVE_BACKWARD

namespace KaitoTokyo {
namespace Logger {

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
	 * @brief Logs an exception with context and (if available) a stack trace.
	 *
	 * Guaranteed not to throw an exception. This method is safe to call
	 * from within a catch block. If `HAVE_BACKWARD` is defined, it will
	 * print a full stack trace.
	 *
	 * @param e The exception that was caught.
	 * @param context A string view describing the context where the exception occurred.
	 */
	void logException(const std::exception &e, std::string_view context) const noexcept
	try {
#ifdef HAVE_BACKWARD
		std::stringstream ss;
		ss << context.data() << ": " << e.what() << "\n";

		backward::StackTrace st;
		st.load_here(32);

		backward::Printer p;
		p.print(st, ss);

		error("--- Stack Trace ---\n{}", ss.str());
#else  // !HAVE_BACKWARD
		error("{}: {}", context, e.what());
#endif // HAVE_BACKWARD
	} catch (const std::exception &log_ex) {
		// Fallback if logging the stack trace itself fails
		error("[LOGGER FATAL] Failed during exception logging: %s\n", log_ex.what());
	} catch (...) {
		error("[LOGGER FATAL] Unknown error during exception logging.");
	}

	/**
	 * @brief Returns true if this logger is an invalid logger.
	 */
	virtual bool isInvalid() const noexcept { return false; }

protected:
	/**
	 * @brief Defines the severity levels for log messages.
	 */
	enum class LogLevel : std::int8_t {
		Debug, ///< Detailed information for debugging.
		Info,  ///< General informational messages.
		Warn,  ///< Warnings about potential issues.
		Error  ///< Error messages for failures.
	};

	/**
	 * @brief The core logging implementation to be provided by a derived class.
	 *
	 * This is the raw sink for the fully formatted log message.
	 * Implementations of this method MUST be `noexcept`.
	 *
	 * @param level The severity level of the message.
	 * @param message The fully formatted log message (including prefix).
	 */
	virtual void log(LogLevel level, std::string_view message) const noexcept = 0;

	/**
	 * @brief Gets the log prefix string (e.g., "[MyPlugin]") to be prepended.
	 *
	 * Implementations of this method MUST be `noexcept`.
	 *
	 * @return A C-string (const char*) representing the prefix.
	 */
	virtual const char *getPrefix() const noexcept = 0;

private:
	/**
	 * @brief Internal helper to format the message and handle all exceptions.
	 *
	 * This function provides the `noexcept` guarantee for all public logging methods.
	 * It formats the prefix and message, then calls the virtual log() function.
	 *
	 * If any exception (e.g., std::bad_alloc) occurs during formatting,
	 * it catches it and logs a fatal error to `stderr` as a fallback,
	 * thus preventing the exception from escaping.
	 *
	 * @tparam Args Parameter pack for format arguments.
	 * @param level The log level.
	 * @param fmt The fmt-style format string.
	 * @param args The arguments to format.
	 */
	template<typename... Args>
	void formatAndLog(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args) const noexcept
	try {
		fmt::memory_buffer buffer;
		// Add prefix and a space
		fmt::format_to(std::back_inserter(buffer), "{} ", getPrefix());
		// Add the user's formatted message
		fmt::vformat_to(std::back_inserter(buffer), fmt, fmt::make_format_args(args...));

		log(level, {buffer.data(), buffer.size()});
	} catch (const std::exception &e) {
		// Fallback: Log formatting failed (e.g., out of memory)
		fprintf(stderr, "[%s] [LOGGER FATAL] Failed to format log message: %s\n", getPrefix(), e.what());
	} catch (...) {
		// Fallback: Unknown, non-standard exception
		fprintf(stderr, "[%s] [LOGGER FATAL] An unknown error occurred while formatting log message.\n",
			getPrefix());
	}
};

} // namespace Logger
} // namespace KaitoTokyo
