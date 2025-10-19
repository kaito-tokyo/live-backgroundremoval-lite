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

#include <cstddef>
#include <string>

#include <opencv2/opencv.hpp>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

using namespace KaitoTokyo::SelfieSegmenter;

const char kParamPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param";
const char kBinPath[] = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin";

const char kTestImage[] = TESTS_DIR "/SelfieSegmenter/selfie001.jpg";
const char kTestImageMask[] = TESTS_DIR "/SelfieSegmenter/selfie001_ncnn.png";

TEST(NcnnSelfieSegmenterTest, Construction)
{
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, 1);
}

TEST(NcnnSelfieSegmenterTest, ProcessRealImage)
{
	// Set up
	int width = 256;
	int height = 144;

	cv::Mat bgrImage = cv::imread(kTestImage, cv::IMREAD_COLOR), bgraImage;
	cv::cvtColor(bgrImage, bgraImage, cv::COLOR_BGR2BGRA);
	ASSERT_FALSE(bgraImage.empty());
	ASSERT_EQ(bgraImage.cols, width);
	ASSERT_EQ(bgraImage.rows, height);
	ASSERT_EQ(bgraImage.channels(), 4);

	cv::Mat refImage = cv::imread(kTestImageMask, cv::IMREAD_GRAYSCALE);
	ASSERT_FALSE(refImage.empty());
	ASSERT_EQ(refImage.cols, width);
	ASSERT_EQ(refImage.rows, height);
	ASSERT_EQ(refImage.channels(), 1);

	// Test
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, 1);
	ASSERT_EQ(selfieSegmenter.getWidth(), static_cast<std::size_t>(width));
	ASSERT_EQ(selfieSegmenter.getHeight(), static_cast<std::size_t>(height));
	ASSERT_EQ(selfieSegmenter.getPixelCount(), static_cast<std::size_t>(width * height));

	selfieSegmenter.process(bgraImage.data);

	// Verify
	cv::Mat maskImage((int)selfieSegmenter.getHeight(), (int)selfieSegmenter.getWidth(), CV_8UC1);
	std::memcpy(maskImage.data, selfieSegmenter.getMask(), selfieSegmenter.getPixelCount());

	EXPECT_GT(cv::countNonZero(maskImage), 0);

	cv::Mat diffImage;
	cv::absdiff(refImage, maskImage, diffImage);
	EXPECT_LT(cv::norm(diffImage, cv::NORM_L1), width * height);
}
