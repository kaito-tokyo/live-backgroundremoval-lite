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

#include <memory>

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
	void operator()(void *ptr) const { bfree(ptr); }
};

/**
 * @brief Deleter for std::unique_ptr that calls obs_data_release()
 * on an obs_data_t pointer.
 */
struct ObsDataDeleter {
	void operator()(obs_data_t *data) const { obs_data_release(data); }
};

/**
 * @brief Deleter for std::unique_ptr that calls obs_data_array_release()
 * on an obs_data_array_t pointer.
 */
struct ObsDataArrayDeleter {
	void operator()(obs_data_array_t *array) const { obs_data_array_release(array); }
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

} // namespace BridgeUtils
} // namespace KaitoTokyo
