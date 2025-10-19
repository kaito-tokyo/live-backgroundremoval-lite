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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>

#include "ISelfieSegmenter.hpp"
#include "MaskBuffer.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

class CoreMLSelfieSegmenterImpl;

class CoreMLSelfieSegmenter : public ISelfieSegmenter {
private:
	constexpr static std::size_t WIDTH = 256;
	constexpr static std::size_t HEIGHT = 144;
	constexpr static std::size_t PIXEL_COUNT = WIDTH * HEIGHT;

	std::unique_ptr<CoreMLSelfieSegmenterImpl> pImpl;

public:
	CoreMLSelfieSegmenter();
	~CoreMLSelfieSegmenter() override;

	std::size_t getWidth() const noexcept override { return WIDTH; }
	std::size_t getHeight() const noexcept override { return HEIGHT; }
	std::size_t getPixelCount() const noexcept override { return PIXEL_COUNT; }

	void process(const std::uint8_t *bgraData) override;

	const std::uint8_t *getMask() const override;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
