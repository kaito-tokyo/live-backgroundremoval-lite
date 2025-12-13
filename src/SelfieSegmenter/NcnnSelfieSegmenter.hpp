/*
 * SelfieSegmenter
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#include "ISelfieSegmenter.hpp"
#include "MaskBuffer.hpp"
#include "ShapeConverter.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

class NcnnSelfieSegmenter final : public ISelfieSegmenter {
public:
	NcnnSelfieSegmenter(const char *paramPath, const char *binPath, int numThreads) : maskBuffer_(kPixelCount)
	{
		selfieSegmenterNet_.opt.num_threads = numThreads;
		selfieSegmenterNet_.opt.use_local_pool_allocator = true;
		selfieSegmenterNet_.opt.openmp_blocktime = 1;

		if (int ret = selfieSegmenterNet_.load_param(paramPath)) {
			throw std::runtime_error("Failed to load selfie segmenter param: " + std::to_string(ret));
		}

		if (int ret = selfieSegmenterNet_.load_model(binPath)) {
			throw std::runtime_error("Failed to load selfie segmenter bin: " + std::to_string(ret));
		}

		inputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 3);
		outputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 1);

		if (inputMat_.empty() || outputMat_.empty()) {
			throw std::runtime_error("Failed to create NcnnSelfieSegmenter internal mats");
		}
	}

	~NcnnSelfieSegmenter() noexcept override = default;

	std::size_t getWidth() const noexcept override { return kWidth; }
	std::size_t getHeight() const noexcept override { return kHeight; }
	std::size_t getPixelCount() const noexcept override { return kPixelCount; }

	void process(const std::uint8_t *bgraData) override
	{
		if (!bgraData) {
			throw std::invalid_argument(
				"NcnnSelfieSegmenter::process received null bgraData; expected non-null pointer to 4 * pixelCount (" +
				std::to_string(getPixelCount()) + ") uint8_ts");
		}

		if (inputMat_.empty() || outputMat_.empty()) {
			throw std::runtime_error("NcnnSelfieSegmenter internal mats are not properly initialized");
		}

		copy_r8_bgra_to_float_chw(inputMat_.channel(0), inputMat_.channel(1), inputMat_.channel(2), bgraData,
					  getPixelCount());

		ncnn::Extractor ex = selfieSegmenterNet_.create_extractor();
		ex.input("in0", inputMat_);
		ex.extract("out0", outputMat_);

		maskBuffer_.write([this](std::uint8_t *mask) {
			copy_float32_to_r8(mask, outputMat_.channel(0), getPixelCount());
		});
	}

	const std::uint8_t *getMask() const override { return maskBuffer_.read(); }

private:
	constexpr static std::size_t kWidth = 256;
	constexpr static std::size_t kHeight = 144;
	constexpr static std::size_t kPixelCount = kWidth * kHeight;

	MaskBuffer maskBuffer_;

	ncnn::Net selfieSegmenterNet_;
	ncnn::Mat inputMat_;
	ncnn::Mat outputMat_;
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
