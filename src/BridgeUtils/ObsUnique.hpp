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
namespace ObsUnique {

struct BfreeDeleter {
	void operator()(void *ptr) const { bfree(ptr); }
};

struct ObsDataDeleter {
	void operator()(obs_data_t *data) const { obs_data_release(data); }
};

struct ObsDataArrayDeleter {
	void operator()(obs_data_array_t *array) const { obs_data_array_release(array); }
};

} // namespace ObsUnique

using unique_bfree_char_t = std::unique_ptr<char, ObsUnique::BfreeDeleter>;

inline unique_bfree_char_t unique_obs_module_file(const char *file)
{
	return unique_bfree_char_t(obs_module_file(file));
}

using unique_obs_data_t = std::unique_ptr<obs_data_t, ObsUnique::ObsDataDeleter>;
using unique_obs_data_array_t = std::unique_ptr<obs_data_array_t, ObsUnique::ObsDataArrayDeleter>;

} // namespace BridgeUtils
} // namespace KaitoTokyo
