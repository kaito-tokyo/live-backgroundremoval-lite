/*
obs-bridge-utils
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

namespace kaito_tokyo {
namespace obs_bridge_utils {

struct bfree_deleter {
	void operator()(void *ptr) const { bfree(ptr); }
};

using unique_bfree_char_t = std::unique_ptr<char, bfree_deleter>;

inline unique_bfree_char_t unique_obs_module_file(const char *file)
{
	return unique_bfree_char_t(obs_module_file(file));
}

} // namespace obs_bridge_utils
} // namespace kaito_tokyo
