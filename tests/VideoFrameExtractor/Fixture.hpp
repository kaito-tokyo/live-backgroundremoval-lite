/*
obs-showdraw
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

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct ColorRGBA {
	std::uint8_t r, g, b, a;
};

constexpr ColorRGBA RED = {255, 0, 0, 255};
constexpr ColorRGBA GREEN = {0, 255, 0, 255};
constexpr ColorRGBA BLUE = {0, 0, 255, 255};
constexpr ColorRGBA WHITE = {255, 255, 255, 255};
constexpr ColorRGBA BLACK = {0, 0, 0, 255};
constexpr ColorRGBA GRAY = {128, 128, 128, 255};
constexpr ColorRGBA YELLOW = {255, 255, 0, 255};
constexpr ColorRGBA CYAN = {0, 255, 255, 255};

constexpr std::array<ColorRGBA, 8> TEST_IMAGE0_ROW_PATTERN = {RED, GREEN, BLUE, WHITE, BLACK, GRAY, YELLOW, CYAN};

const std::size_t TEST_IMAGE0_WIDTH = 8;
const std::size_t TEST_IMAGE0_HEIGHT = 8;

constexpr auto generate_test_image_data()
{
	constexpr auto num_pixels = TEST_IMAGE0_WIDTH * TEST_IMAGE0_HEIGHT;
	std::array<std::uint8_t, num_pixels * 4> data{}; // 全てゼロで初期化

	for (std::size_t y = 0; y < TEST_IMAGE0_HEIGHT; ++y) {
		for (std::size_t x = 0; x < TEST_IMAGE0_WIDTH; ++x) {
			const auto &color = TEST_IMAGE0_ROW_PATTERN[x];
			const std::size_t base_index = (y * TEST_IMAGE0_WIDTH + x) * 4;
			data[base_index + 0] = color.r;
			data[base_index + 1] = color.g;
			data[base_index + 2] = color.b;
			data[base_index + 3] = color.a;
		}
	}
	return data;
}

const auto TEST_IMAGE0_DATA_ARRAY = generate_test_image_data();
const std::uint8_t *const TEST_IMAGE0_DATA = TEST_IMAGE0_DATA_ARRAY.data();

struct ColorYUV {
	std::uint8_t y, u, v;
};
constexpr ColorYUV rgb_to_yuv(const ColorRGBA &c)
{
	const int y = ((66 * c.r + 129 * c.g + 25 * c.b + 128) >> 8) + 16;
	const int u = ((-38 * c.r - 74 * c.g + 112 * c.b + 128) >> 8) + 128;
	const int v = ((112 * c.r - 94 * c.g - 18 * c.b + 128) >> 8) + 128;
	return {static_cast<std::uint8_t>(y < 0 ? 0 : (y > 255 ? 255 : y)),
		static_cast<std::uint8_t>(u < 0 ? 0 : (u > 255 ? 255 : u)),
		static_cast<std::uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v))};
}

constexpr auto generate_uyvy_data() {
    std::array<std::uint8_t, WIDTH * HEIGHT * 2> uyvy_data{};
    for (std::size_t y = 0; y < HEIGHT; ++y) {
        for (std::size_t x = 0; x < WIDTH; x += 2) { // 2ピクセルずつ処理
            // 元のピクセルを取得
            const auto& rgba0 = TEST_IMAGE0_ROW_PATTERN[x];
            const auto& rgba1 = TEST_IMAGE0_ROW_PATTERN[x + 1];

            // YUVに変換
            const auto yuv0 = rgb_to_yuv(rgba0);
            const auto yuv1 = rgb_to_yuv(rgba1);

            // UとVを2ピクセルで平均化 (4:2:2 subsampling)
            const std::uint8_t u_avg = (yuv0.u + yuv1.u) / 2;
            const std::uint8_t v_avg = (yuv0.v + yuv1.v) / 2;

            // UYVYフォーマットで格納
            const std::size_t i = (y * WIDTH + x) * 2;
            uyvy_data[i + 0] = u_avg;
            uyvy_data[i + 1] = yuv0.y;
            uyvy_data[i + 2] = v_avg;
            uyvy_data[i + 3] = yuv1.y;
        }
    }
    return uyvy_data;
}