/*
 * SelfieSegmenter
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
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

	virtual void process(const std::uint8_t *bgraData) = 0;

	virtual const std::uint8_t *getMask() const = 0;

	ISelfieSegmenter(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter &operator=(const ISelfieSegmenter &) = delete;
	ISelfieSegmenter(ISelfieSegmenter &&) = delete;
	ISelfieSegmenter &operator=(ISelfieSegmenter &&) = delete;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
