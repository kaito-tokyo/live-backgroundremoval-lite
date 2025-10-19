/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <new>

#include <memory_resource>

namespace KaitoTokyo {
namespace SelfieSegmenter {

class ForceAlignmentResource : public std::pmr::memory_resource {
private:
	std::pmr::memory_resource *upstream;
	std::size_t required_alignment;

public:
	explicit ForceAlignmentResource(std::size_t _alignment, std::pmr::memory_resource *_upstream)
		: upstream(_upstream),
		  required_alignment(_alignment)
	{
	}

protected:
	void *do_allocate(std::size_t bytes, std::size_t) override
	{
		return upstream->allocate(bytes, required_alignment);
	}

	void do_deallocate(void *p, std::size_t bytes, std::size_t) override
	{
		upstream->deallocate(p, bytes, required_alignment);
	}

	bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override { return this == &other; }
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
