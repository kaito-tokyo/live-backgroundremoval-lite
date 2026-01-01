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

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace KaitoTokyo::SelfieSegmenter {

class ISelfieSegmenter {
protected:
	ISelfieSegmenter() = default;

public:
	virtual ~ISelfieSegmenter() = default;

	virtual std::size_t getWidth() const noexcept = 0;
	virtual std::size_t getHeight() const noexcept = 0;
	virtual std::size_t getPixelCount() const noexcept = 0;

	virtual void process(const std::uint8_t *bgraData) = 0;

	virtual const std::uint8_t *getMask() const = 0;

	ISelfieSegmenter(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter &operator=(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter(ISelfieSegmenter &&) = delete;
	ISelfieSegmenter &operator=(ISelfieSegmenter &&) = delete;
};

} // namespace KaitoTokyo::SelfieSegmenter
