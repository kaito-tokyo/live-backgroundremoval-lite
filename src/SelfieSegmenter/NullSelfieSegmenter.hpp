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

#include "ISelfieSegmenter.hpp"
#include "MaskBuffer.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

class NullSelfieSegmenter : public ISelfieSegmenter {
public:
	NullSelfieSegmenter() {}

	~NullSelfieSegmenter() override = default;

	std::size_t getWidth() const noexcept override { return 0; }
	std::size_t getHeight() const noexcept override { return 0; }
	std::size_t getPixelCount() const noexcept override { return 0; }
	const char *getBackendName() const noexcept override { return "null"; }

	void process(const std::uint8_t *) override {}

	const std::uint8_t *getMask() const override { return nullptr; }

	NullSelfieSegmenter(const NullSelfieSegmenter &) = delete;
	NullSelfieSegmenter &operator=(const NullSelfieSegmenter &) = delete;
	NullSelfieSegmenter(NullSelfieSegmenter &&) = delete;
	NullSelfieSegmenter &operator=(NullSelfieSegmenter &&) = delete;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
