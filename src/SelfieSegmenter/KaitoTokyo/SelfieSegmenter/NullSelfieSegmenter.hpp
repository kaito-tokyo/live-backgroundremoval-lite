/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo SelfieSegmenter Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "ISelfieSegmenter.hpp"
#include "MaskBuffer.hpp"

namespace KaitoTokyo::SelfieSegmenter {

class NullSelfieSegmenter final : public ISelfieSegmenter {
public:
	NullSelfieSegmenter() : maskBuffer_(kPixelCount) {}

	~NullSelfieSegmenter() noexcept override = default;

	std::size_t getWidth() const noexcept override { return kWidth; }
	std::size_t getHeight() const noexcept override { return kHeight; }
	std::size_t getPixelCount() const noexcept override { return kPixelCount; }

	void process(const std::uint8_t *) override {}

	const std::uint8_t *getMask() const override { return maskBuffer_.read(); }

	NullSelfieSegmenter(const NullSelfieSegmenter &) = delete;
	NullSelfieSegmenter &operator=(const NullSelfieSegmenter &) = delete;
	NullSelfieSegmenter(NullSelfieSegmenter &&) = delete;
	NullSelfieSegmenter &operator=(NullSelfieSegmenter &&) = delete;

private:
	constexpr static std::size_t kWidth = 256;
	constexpr static std::size_t kHeight = 144;
	constexpr static std::size_t kPixelCount = kWidth * kHeight;

	MaskBuffer maskBuffer_;
};

} // namespace KaitoTokyo::SelfieSegmenter
