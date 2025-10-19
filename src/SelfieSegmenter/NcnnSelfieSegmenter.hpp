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

class NcnnSelfieSegmenter : public ISelfieSegmenter {
private:
	constexpr static std::size_t kWidth = 256;
	constexpr static std::size_t kHeight = 144;
	constexpr static std::size_t kPixelCount = kWidth * kHeight;

	MaskBuffer maskBuffer;

	ncnn::Net selfieSegmenterNet;
	ncnn::Mat inputMat;
	ncnn::Mat outputMat;

public:
	NcnnSelfieSegmenter(const char *paramPath, const char *binPath, int numThreads, int ncnnGpuIndex = -1)
		: maskBuffer(kPixelCount)
	{
		selfieSegmenterNet.opt.use_vulkan_compute = ncnnGpuIndex >= 0;
		selfieSegmenterNet.opt.num_threads = numThreads;
		selfieSegmenterNet.opt.use_local_pool_allocator = true;
		selfieSegmenterNet.opt.openmp_blocktime = 1;

		if (int ret = selfieSegmenterNet.load_param(paramPath)) {
			throw std::runtime_error("Failed to load selfie segmenter param: " + std::to_string(ret));
		}

		if (int ret = selfieSegmenterNet.load_model(binPath)) {
			throw std::runtime_error("Failed to load selfie segmenter bin: " + std::to_string(ret));
		}

		inputMat.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 3);
		outputMat.create(static_cast<int>(getWidth()), static_cast<int>(getHeight()), 1);
	}

	~NcnnSelfieSegmenter() override = default;

	std::size_t getWidth() const noexcept override { return kWidth; }
	std::size_t getHeight() const noexcept override { return kHeight; }
	std::size_t getPixelCount() const noexcept override { return kPixelCount; }

	void process(const std::uint8_t *bgraData) override
	{
		if (!bgraData) {
			throw std::invalid_argument("bgraData is null");
		}

		copy_r8_bgra_to_float_chw(inputMat.channel(0), inputMat.channel(1), inputMat.channel(2), bgraData,
					  getPixelCount());

		ncnn::Extractor ex = selfieSegmenterNet.create_extractor();
		ex.input("in0", inputMat);
		ex.extract("out0", outputMat);

		maskBuffer.write([this](std::uint8_t *mask) {
			copy_float32_to_r8(mask, outputMat.channel(0), getPixelCount());
		});
	}

	const std::uint8_t *getMask() const override { return maskBuffer.read(); }
};

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
