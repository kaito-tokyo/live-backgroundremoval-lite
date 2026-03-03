// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

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
