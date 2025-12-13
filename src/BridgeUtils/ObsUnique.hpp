/*
 * BridgeUtils
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file 
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <obs-module.h>

namespace KaitoTokyo {
namespace BridgeUtils {

/**
 * @brief Contains custom deleters for managing OBS-specific resource pointers
 * with std::unique_ptr.
 */
namespace ObsUnique {

/**
 * @brief Deleter for std::unique_ptr that calls bfree() on the pointer.
 * Used for memory allocated by OBS functions like obs_module_file.
 */
struct BfreeDeleter {
	/**
	 * @brief Frees the memory using bfree.
	 * @param ptr Pointer to the memory allocated by OBS.
	 */
	void operator()(void *ptr) const noexcept
	{
		// bfree is a C function, safe to assume it doesn't throw.
		bfree(ptr);
	}
};

/**
 * @brief Deleter for std::unique_ptr that calls obs_data_release()
 * on an obs_data_t pointer.
 */
struct ObsDataDeleter {
	/**
	 * @brief Releases the obs_data_t object.
	 * @param data Pointer to the OBS data object.
	 */
	void operator()(obs_data_t *data) const noexcept
	{
		// obs_data_release is a C function, safe to assume it doesn't throw.
		obs_data_release(data);
	}
};

/**
 * @brief Deleter for std::unique_ptr that calls obs_data_array_release()
 * on an obs_data_array_t pointer.
 */
struct ObsDataArrayDeleter {
	/**
	 * @brief Releases the obs_data_array_t object.
	 * @param array Pointer to the OBS data array object.
	 */
	void operator()(obs_data_array_t *array) const noexcept
	{
		// obs_data_array_release is a C function, safe to assume it doesn't throw.
		obs_data_array_release(array);
	}
};

} // namespace ObsUnique

/**
 * @brief A std::unique_ptr for a char pointer that will be freed using bfree().
 */
using unique_bfree_char_t = std::unique_ptr<char, ObsUnique::BfreeDeleter>;

/**
 * @brief A std::unique_ptr for an obs_data_t object.
 */
using unique_obs_data_t = std::unique_ptr<obs_data_t, ObsUnique::ObsDataDeleter>;

/**
 * @brief A std::unique_ptr for an obs_data_array_t object.
 */
using unique_obs_data_array_t = std::unique_ptr<obs_data_array_t, ObsUnique::ObsDataArrayDeleter>;

/**
 * @brief Factory function to create a unique_bfree_char_t for a module file path.
 * Wraps obs_module_file.
 *
 * @param file The relative file name to find within the module's data path.
 * @return A valid (non-null) unique_bfree_char_t managing the allocated path string.
 * @throws std::runtime_error If obs_module_file fails (e.g., returns null).
 * This function throws on failure and **never returns an empty (null) pointer.**
 */
[[nodiscard]]
inline unique_bfree_char_t unique_obs_module_file(const char *file)
{
	char *rawPath = obs_module_file(file);
	if (!rawPath) {
		// Provide a more informative error message
		const std::string errorMsg = file ? std::string("obs_module_file failed for file: ") + file
						  : "obs_module_file failed (null input path)";
		throw std::runtime_error(errorMsg);
	}
	return unique_bfree_char_t(rawPath);
}

/**
 * @brief Factory function to create a unique_bfree_char_t for a module config path.
 * Wraps obs_module_config_path.
 *
 * @param file The relative file name to find within the module's config path.
 * @return A valid (non-null) unique_bfree_char_t managing the allocated path string.
 * @throws std::runtime_error If obs_module_config_path fails (e.g., returns null).
 * This function throws on failure and **never returns an empty (null) pointer.**
 */
[[nodiscard]]
inline unique_bfree_char_t unique_obs_module_config_path(const char *file)
{
	char *rawPath = obs_module_config_path(file);
	if (!rawPath) {
		const std::string errorMsg = file ? std::string("obs_module_config_path failed for file: ") + file
						  : "obs_module_config_path failed (null input path)";
		throw std::runtime_error(errorMsg);
	}
	return unique_bfree_char_t(rawPath);
}

} // namespace BridgeUtils
} // namespace KaitoTokyo
