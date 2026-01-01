/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo SelfieSegmenter Library - Tests
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

#include <gtest/gtest.h>

#include <KaitoTokyo/SelfieSegmenter/NcnnSelfieSegmenter.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#ifdef PREFIXED_NCNN_HEADERS
#include <ncnn/net.h>
#else
#include <net.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace KaitoTokyo::SelfieSegmenter;

const std::filesystem::path kParamPath = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param";
const std::filesystem::path kBinPath = DATA_DIR "/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin";

const char kTestImage[] = TESTS_DIR "/SelfieSegmenter/selfie001.jpg";
const char kTestImageMask[] = TESTS_DIR "/SelfieSegmenter/selfie001_ncnn.png";

#include <iostream>

TEST(NcnnSelfieSegmenterTest, Construction)
{
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, 1);
}

TEST(NcnnSelfieSegmenterTest, ProcessRealImage)
{
	const int width = 256;
	const int height = 144;

	int testImageX, testImageY, testImageChannels;
	std::uint8_t *testImageRGB = stbi_load(kTestImage, &testImageX, &testImageY, &testImageChannels, 3);
	ASSERT_EQ(testImageX, width);
	ASSERT_EQ(testImageY, height);
	ASSERT_EQ(testImageChannels, 3);

	std::vector<std::uint8_t> testImageBGRA(testImageX * testImageY * 4);
	for (int i = 0; i < testImageX * testImageY; i++) {
		testImageBGRA[i * 4 + 0] = testImageRGB[i * 3 + 2];
		testImageBGRA[i * 4 + 1] = testImageRGB[i * 3 + 1];
		testImageBGRA[i * 4 + 2] = testImageRGB[i * 3 + 0];
		testImageBGRA[i * 4 + 3] = 255;
	}

	int refImageX, refImageY, refImageChannels;
	std::uint8_t *refImageData = stbi_load(kTestImageMask, &refImageX, &refImageY, &refImageChannels, 1);
	ASSERT_EQ(refImageX, width);
	ASSERT_EQ(refImageY, height);
	ASSERT_EQ(refImageChannels, 1);

	// Test
	NcnnSelfieSegmenter selfieSegmenter(kParamPath, kBinPath, 1);
	ASSERT_EQ(selfieSegmenter.getWidth(), static_cast<std::size_t>(width));
	ASSERT_EQ(selfieSegmenter.getHeight(), static_cast<std::size_t>(height));
	ASSERT_EQ(selfieSegmenter.getPixelCount(), static_cast<std::size_t>(width * height));

	selfieSegmenter.process(testImageBGRA.data());

	const std::uint8_t *resultImage = selfieSegmenter.getMask();
	ASSERT_EQ(selfieSegmenter.getWidth(), width);
	ASSERT_EQ(selfieSegmenter.getHeight(), height);
	ASSERT_EQ(selfieSegmenter.getPixelCount(), static_cast<std::size_t>(width * height));

	std::uint64_t totalDiff = 0;
	for (std::size_t i = 0; i < selfieSegmenter.getPixelCount(); i++) {
		totalDiff += std::abs(resultImage[i] - refImageData[i]);
	}
	EXPECT_LT(totalDiff, width * height);
}
