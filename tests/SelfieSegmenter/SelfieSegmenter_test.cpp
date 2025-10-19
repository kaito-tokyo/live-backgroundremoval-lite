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

#include <gtest/gtest.h>

#include <NcnnSelfieSegmenter.hpp>

#include <vector>
#include <string>
#include <cstdint>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#include <opencv2/opencv.hpp>

using namespace KaitoTokyo::SelfieSegmenter;

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

const char kParamPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param";
const char kBinPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin";
const char kTestImage[] = TESTS_DIR "/SelfieSegmenter/selfie001.jpg";
const char kTestImageMask[] = TESTS_DIR "/SelfieSegmenter/selfie001_mask.png";

TEST(SelfieSegmenterTest, Construction)
{
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, -1, 1);
	EXPECT_EQ(selfieSegmenter.getMask().size(), selfieSegmenter.getPixelCount());
}

TEST(SelfieSegmenterTest, ProcessRealImage)
{
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, -1, 1);
	std::vector<uint8_t> bgra;
	int w = 0, h = 0;
	ASSERT_TRUE(load_jpg_bgra(kTestImage, bgra, w, h));
	ASSERT_EQ(w, selfieSegmenter.getWidth());
	ASSERT_EQ(h, selfieSegmenter.getHeight());
	selfieSegmenter.process(bgra.data());
	const auto &mask = selfieSegmenter.getMask();
	// Check mask is not all zero
	int nonzero = 0;
	for (auto v : mask)
		nonzero += (v != 0);
	EXPECT_GT(nonzero, 0);

	// Load reference mask image (PNG, grayscale)
	cv::Mat ref_img = cv::imread(kTestImageMask, cv::IMREAD_GRAYSCALE);
	ASSERT_FALSE(ref_img.empty());
	ASSERT_EQ(ref_img.cols, selfieSegmenter.getWidth());
	ASSERT_EQ(ref_img.rows, selfieSegmenter.getHeight());
	// Compare pixel-wise
	int same = 0;
	for (std::size_t i = 0; i < selfieSegmenter.getPixelCount(); ++i) {
		if (std::abs(int(mask[i]) - int(ref_img.data[i])) <= 1)
			++same;
	}
	double ratio = double(same) / selfieSegmenter.getPixelCount();
	EXPECT_GE(ratio, 0.95);
}
