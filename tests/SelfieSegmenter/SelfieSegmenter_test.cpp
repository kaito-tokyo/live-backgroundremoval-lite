/*
Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <gtest/gtest.h>
#include <SelfieSegmenter/SelfieSegmenter.hpp>
#include <ncnn/net.h>
#include <vector>
#include <string>
#include <cstdint>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// JPEG(BGR) -> BGRA loader
static bool load_jpg_bgra(const std::string &filename, std::vector<uint8_t> &out_bgra, int &width, int &height)
{
	int n;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &n, 4);
	if (!data)
		return false;
	out_bgra.assign(data, data + width * height * 4);
	stbi_image_free(data);
	return true;
}

using namespace KaitoTokyo::BackgroundRemovalLite;

const char kParamPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation.ncnn.param";
const char kBinPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation.ncnn.bin";
const char kTestImage[] = TESTS_DIR "/SelfieSegmenter/selfie001.jpg";
const char kTestImageMask[] = TESTS_DIR "/SelfieSegmenter/selfie001_mask.png";

TEST(SelfieSegmenterTest, Construction)
{
	ncnn::Net net;
	ASSERT_EQ(net.load_param(kParamPath), 0);
	ASSERT_EQ(net.load_model(kBinPath), 0);
	SelfieSegmenter segmenter(net);
	EXPECT_EQ(segmenter.maskBuffer.read().size(), SelfieSegmenter::PIXEL_COUNT);
}

TEST(SelfieSegmenterTest, ProcessRealImage)
{
	ncnn::Net net;
	ASSERT_EQ(net.load_param(kParamPath), 0);
	ASSERT_EQ(net.load_model(kBinPath), 0);
	SelfieSegmenter segmenter(net);
	std::vector<uint8_t> bgra;
	int w = 0, h = 0;
	ASSERT_TRUE(load_jpg_bgra(kTestImage, bgra, w, h));
	ASSERT_EQ(w, SelfieSegmenter::INPUT_WIDTH);
	ASSERT_EQ(h, SelfieSegmenter::INPUT_HEIGHT);
	segmenter.process(bgra.data());
	const auto &mask = segmenter.getMask();
	// Check mask is not all zero
	int nonzero = 0;
	for (auto v : mask)
		nonzero += (v != 0);
	EXPECT_GT(nonzero, 0);

	// Load reference mask image (PNG, grayscale)
	int ref_w = 0, ref_h = 0, ref_c = 0;
	unsigned char *ref_data = stbi_load(kTestImageMask, &ref_w, &ref_h, &ref_c, 1);
	ASSERT_TRUE(ref_data != nullptr);
	ASSERT_EQ(ref_w, SelfieSegmenter::INPUT_WIDTH);
	ASSERT_EQ(ref_h, SelfieSegmenter::INPUT_HEIGHT);
	// Compare pixel-wise
	int same = 0;
	for (int i = 0; i < SelfieSegmenter::PIXEL_COUNT; ++i) {
		if (std::abs(int(mask[i]) - int(ref_data[i])) <= 1)
			++same;
	}
	stbi_image_free(ref_data);
	double ratio = double(same) / SelfieSegmenter::PIXEL_COUNT;
	EXPECT_GE(ratio, 0.95);
}
