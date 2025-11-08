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
#include <iostream>

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

class MediapipeLandscapeNcnnSelfieSegmenter : public ISelfieSegmenter {
public:
	MediapipeLandscapeNcnnSelfieSegmenter(const char *paramPath, const char *binPath, int numThreads) : maskBuffer_(kPixelCount)
	{
		net_.opt.num_threads = numThreads;
		net_.opt.use_local_pool_allocator = true;
		net_.opt.openmp_blocktime = 1;

		if (int ret = net_.load_param(paramPath)) {
			throw std::runtime_error("Failed to load selfie segmenter param: " + std::to_string(ret));
		}

		if (int ret = net_.load_model(binPath)) {
			throw std::runtime_error("Failed to load selfie segmenter bin: " + std::to_string(ret));
		}

		inputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 3);
		outputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 1);
	}

	~MediapipeLandscapeNcnnSelfieSegmenter() noexcept override = default;

	std::size_t getWidth() const noexcept override { return kWidth; }
	std::size_t getHeight() const noexcept override { return kHeight; }
	std::size_t getPixelCount() const noexcept override { return kPixelCount; }

	void process(const std::uint8_t *bgraData) override
	{
		if (!bgraData) {
			throw std::invalid_argument(
				"MediapipeLandscapeNcnnSelfieSegmenter::process received null bgraData; expected non-null pointer to 4 * pixelCount (" +
				std::to_string(getPixelCount()) + ") bytes");
		}

		copy_r8_bgra_to_float_chw(inputMat_.channel(0), inputMat_.channel(1), inputMat_.channel(2), bgraData,
					  getPixelCount());

		ncnn::Extractor ex = net_.create_extractor();
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

	ncnn::Net net_;
	ncnn::Mat inputMat_;
	ncnn::Mat outputMat_;
};

class PPHumansegv2LiteNcnnSelfieSegmenter : public ISelfieSegmenter {
public:
	PPHumansegv2LiteNcnnSelfieSegmenter(const char *paramPath, const char *binPath, int numThreads) : maskBuffer_(kPixelCount)
	{
		net_.opt.num_threads = numThreads;
		net_.opt.use_local_pool_allocator = true;
		net_.opt.openmp_blocktime = 1;

		if (int ret = net_.load_param(paramPath)) {
			throw std::runtime_error("Failed to load selfie segmenter param: " + std::to_string(ret));
		}

		if (int ret = net_.load_model(binPath)) {
			throw std::runtime_error("Failed to load selfie segmenter bin: " + std::to_string(ret));
		}

		inputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 3);
		outputMat_.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 2);
	}

	~PPHumansegv2LiteNcnnSelfieSegmenter() noexcept override = default;

	std::size_t getWidth() const noexcept override { return kWidth; }
	std::size_t getHeight() const noexcept override { return kHeight; }
	std::size_t getPixelCount() const noexcept override { return kPixelCount; }

	void process(const std::uint8_t *bgraData) override
	{
		if (!bgraData) {
			throw std::invalid_argument(
				"MediapipeLandscapeNcnnSelfieSegmenter::process received null bgraData; expected non-null pointer to 4 * pixelCount (" +
				std::to_string(getPixelCount()) + ") bytes");
		}

		copy_r8_bgra_to_float_chw(inputMat_.channel(0), inputMat_.channel(1), inputMat_.channel(2), bgraData,
					  getPixelCount());

		ncnn::Extractor ex = net_.create_extractor();
		ex.input("in0", inputMat_);
		ex.extract("out0", outputMat_);

		maskBuffer_.write([this](std::uint8_t *mask) {
			for (std::size_t i = 0; i < getPixelCount(); i++) {
				std::cout << "outputMat_.channel(0)[" << i << "] = " << outputMat_.channel(0)[i] << std::endl;
				std::cout << "outputMat_.channel(1)[" << i << "] = " << outputMat_.channel(1)[i] << std::endl;
				mask[i] = outputMat_.channel(0)[i] > outputMat_.channel(1)[i] ? 255 : 0;
			}
		});
	}

	const std::uint8_t *getMask() const override { return maskBuffer_.read(); }

private:
	constexpr static std::size_t kWidth = 256;
	constexpr static std::size_t kHeight = 144;
	constexpr static std::size_t kPixelCount = kWidth * kHeight;

	MaskBuffer maskBuffer_;

	ncnn::Net net_;
	ncnn::Mat inputMat_;
	ncnn::Mat outputMat_;
};


} // namespace SelfieSegmenter
} // namespace KaitoTokyo
