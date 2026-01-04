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

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#include <KaitoTokyo/Memory/AlignedAllocator.hpp>

#include "ISelfieSegmenter.hpp"
#include "MaskBuffer.hpp"
#include "ShapeConverter.hpp"

namespace KaitoTokyo::SelfieSegmenter {

namespace {

constexpr auto kMaxFileSize = 10 * 1024 * 1024;
constexpr auto kAlignment = 32;

} // anonymous namespace

class NcnnSelfieSegmenter final : public ISelfieSegmenter {
public:
	NcnnSelfieSegmenter(const char *paramText, int binSize, const unsigned char *binData, int numThreads)
		: maskBuffer_(kPixelCount)
	{
		selfieSegmenterNet_.opt.num_threads = numThreads;
		selfieSegmenterNet_.opt.use_local_pool_allocator = true;
		selfieSegmenterNet_.opt.openmp_blocktime = 1;

		if (selfieSegmenterNet_.load_param_mem(paramText) != 0) {
			throw std::runtime_error("ParamLoadError(NcnnSelfieSegmenter::NcnnSelfieSegmenter)");
		}

		if (selfieSegmenterNet_.load_model(binData) != binSize) {
			throw std::runtime_error("ModelLoadError(NcnnSelfieSegmenter::NcnnSelfieSegmenter)");
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

	std::vector<unsigned char, Memory::AlignedAllocator<unsigned char>> binBuffer_{
		0, Memory::AlignedAllocator<unsigned char>(kAlignment)};
	ncnn::Net selfieSegmenterNet_;
	ncnn::Mat inputMat_;
	ncnn::Mat outputMat_;
};

} // namespace KaitoTokyo::SelfieSegmenter
