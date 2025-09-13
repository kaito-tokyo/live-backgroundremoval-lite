/*
OBS Background Removal Lite
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

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <net.h>

TEST(StubTest, Stub)
{
	ncnn::Net net;
	net.load_param("data/models/mediapipe_selfie_segmentation.ncnn.param");
	net.load_model("data/models/mediapipe_selfie_segmentation.ncnn.bin");

	cv::Mat bgr_image = cv::imread("test.png", cv::IMREAD_COLOR);

	const int target_size = 256;
	int img_w = bgr_image.cols;
	int img_h = bgr_image.rows;

	const float mean_vals[3] = {127.5f, 127.5f, 127.5f};
	const float norm_vals[3] = {1.0f / 127.5f, 1.0f / 127.5f, 1.0f / 127.5f};

	ncnn::Mat in = ncnn::Mat::from_pixels_resize(bgr_image.data, ncnn::Mat::PIXEL_BGR2RGB, img_w, img_h,
						     target_size, target_size);

	in.substract_mean_normalize(mean_vals, norm_vals);

	ncnn::Extractor ex = net.create_extractor();
	ex.input("in0", in);

	ncnn::Mat out;
	ex.extract("out0", out);
	printf("Output shape: w=%d, h=%d, c=%d, dims=%d\n", out.w, out.h, out.c, out.dims);

	// ncnn::MatからOpenCVのMatに変換
	// 出力は0.0~1.0の確率値なので、255を掛けてグレースケール画像にする
	cv::Mat mask(out.h, out.w, CV_32FC1); // まずはfloat型でデータを受け取る
	memcpy(mask.data, (float *)out.data, out.w * out.h * sizeof(float));

	// 0-255の範囲に変換し、8bitのグレースケール画像にする
	cv::Mat mask_8u;
	mask.convertTo(mask_8u, CV_8UC1, 255.0);

	// 6. マスクを元の画像サイズにリサイズ
	cv::Mat resized_mask;
	cv::resize(mask_8u, resized_mask, cv::Size(img_w, img_h), 0, 0, cv::INTER_LINEAR);

	// (オプション) よりくっきりしたマスクにするために閾値処理を追加
	cv::Mat binary_mask = resized_mask.clone();

	cv::Mat masked_image;
	bgr_image.copyTo(masked_image, binary_mask);

	cv::imwrite("mask_output.png", binary_mask);
	cv::imwrite("masked_image_output.png", masked_image);
}
