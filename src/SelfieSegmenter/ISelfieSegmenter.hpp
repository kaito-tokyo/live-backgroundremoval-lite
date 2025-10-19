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

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace KaitoTokyo {
namespace SelfieSegmenter {

class ISelfieSegmenter {
protected:
	ISelfieSegmenter() = default;

public:
	virtual ~ISelfieSegmenter() = default;

	virtual std::size_t getWidth() const noexcept = 0;
	virtual std::size_t getHeight() const noexcept = 0;
	virtual std::size_t getPixelCount() const noexcept = 0;
	virtual const char *getBackendName() const noexcept = 0;

	virtual void process(const std::uint8_t *bgraData) = 0;

	virtual const std::uint8_t *getMask() const = 0;

	ISelfieSegmenter(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter &operator=(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter(ISelfieSegmenter &&) = delete;
	ISelfieSegmenter &operator=(ISelfieSegmenter &&) = delete;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
