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

#include <SelfieSegmenter.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <cstdint>

#include <ncnn/net.h>
#include <opencv2/opencv.hpp>

// JPEG(BGR) -> BGRA loader using OpenCV
static bool load_jpg_bgra(const std::string &filename, std::vector<uint8_t> &out_bgra, int &width, int &height)
{
	cv::Mat img = cv::imread(filename, cv::IMREAD_COLOR);
	if (img.empty())
		return false;
	cv::Mat img_bgra;
	cv::cvtColor(img, img_bgra, cv::COLOR_BGR2BGRA);
	width = img_bgra.cols;
	height = img_bgra.rows;
	out_bgra.assign(img_bgra.data, img_bgra.data + width * height * 4);
	return true;
}

using namespace KaitoTokyo::LiveBackgroundRemovalLite;

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
	EXPECT_EQ(segmenter.getMask().size(), SelfieSegmenter::PIXEL_COUNT);
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
	cv::Mat ref_img = cv::imread(kTestImageMask, cv::IMREAD_GRAYSCALE);
	ASSERT_FALSE(ref_img.empty());
	ASSERT_EQ(ref_img.cols, SelfieSegmenter::INPUT_WIDTH);
	ASSERT_EQ(ref_img.rows, SelfieSegmenter::INPUT_HEIGHT);
	// Compare pixel-wise
	int same = 0;
	for (int i = 0; i < SelfieSegmenter::PIXEL_COUNT; ++i) {
		if (std::abs(int(mask[i]) - int(ref_img.data[i])) <= 1)
			++same;
	}
	double ratio = double(same) / SelfieSegmenter::PIXEL_COUNT;
	EXPECT_GE(ratio, 0.95);
}
