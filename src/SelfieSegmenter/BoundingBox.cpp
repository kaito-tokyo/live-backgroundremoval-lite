/*
 * Live Background Removal Lite - MainFilter Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#if defined(_M_ARM64) || defined(__aarch64__)
#ifdef __ARM_NEON
#define SELFIE_SEGMENTER_HAVE_NEON
#include <arm_neon.h>
#endif // __ARM_NEON
#endif // defined(_M_ARM64) || defined(__aarch64__)

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif // defined(_MSC_VER)
#endif

#include "BoundingBox.hpp"

#include <cstddef>

namespace KaitoTokyo::SelfieSegmenter {

namespace {

#if defined(_MSC_VER)
inline int get_first_bit_index(int mask)
{
	unsigned long index;
	_BitScanForward(&index, mask);
	return (int)index;
}

inline int get_last_bit_index(int mask)
{
	unsigned long index;
	_BitScanReverse(&index, mask);
	return (int)index;
}
#else
inline int get_first_bit_index(int mask)
{
	return __builtin_ctz(mask);
}

inline int get_last_bit_index(int mask)
{
	return 31 - __builtin_clz(mask);
}
#endif

#ifdef __ARM_NEON
inline bool calculateBoundingBoxNEON256x144(BoundingBox *boundingBox, const std::uint8_t *data, std::uint8_t threshold)
{
	constexpr std::uint32_t width = 256;
	constexpr std::uint32_t height = 144;
	constexpr std::int32_t num_blocks = 16;

	const uint8x16_t v_threshold = vdupq_n_u8(threshold);

	uint8x16_t col_acc[num_blocks];
	for (int i = 0; i < num_blocks; ++i) {
		col_acc[i] = vdupq_n_u8(0);
	}

	uint8_t row_flags[height];

	for (std::uint32_t y = 0; y < height; ++y) {
		const uint8_t *row_ptr = data + y * width;
		uint8x16_t row_any = vdupq_n_u8(0);

		for (std::int32_t b = 0; b < num_blocks; ++b) {
			uint8x16_t v_data = vld1q_u8(row_ptr + b * 16);

			uint8x16_t v_cmp = vcgtq_u8(v_data, v_threshold);

			col_acc[b] = vorrq_u8(col_acc[b], v_cmp);
			row_any = vorrq_u8(row_any, v_cmp);
		}

		row_flags[y] = vmaxvq_u8(row_any);
	}

	std::int32_t min_y = -1;
	std::int32_t max_y = -1;

	for (std::uint32_t y = 0; y < height; ++y) {
		if (row_flags[y] != 0) {
			min_y = static_cast<std::int32_t>(y);
			break;
		}
	}

	if (min_y == -1)
		return false;

	for (std::int32_t y = static_cast<std::int32_t>(height) - 1; y >= min_y; --y) {
		if (row_flags[y] != 0) {
			max_y = y;
			break;
		}
	}

	std::int32_t min_x = width;
	std::int32_t max_x = -1;

	for (std::int32_t b = 0; b < num_blocks; ++b) {
		if (vmaxvq_u8(col_acc[b]) == 0)
			continue;

		uint8_t blk[16];
		vst1q_u8(blk, col_acc[b]);

		for (int i = 0; i < 16; ++i) {
			if (blk[i] != 0) {
				min_x = (b * 16) + i;
				goto found_min_x;
			}
		}
	}
found_min_x:;

	for (std::int32_t b = num_blocks - 1; b >= 0; --b) {
		if (vmaxvq_u8(col_acc[b]) == 0)
			continue;

		uint8_t blk[16];
		vst1q_u8(blk, col_acc[b]);

		for (int i = 15; i >= 0; --i) {
			if (blk[i] != 0) {
				max_x = (b * 16) + i;
				goto found_max_x;
			}
		}
	}
found_max_x:;

	if (max_x < min_x)
		return false;

	boundingBox->x = min_x;
	boundingBox->y = min_y;
	boundingBox->width = max_x - min_x + 1;
	boundingBox->height = max_y - min_y + 1;

	return true;
}
#endif // __ARM_NEON

#ifdef __AVX2__
/**
 * @brief Checks if the CPU supports AVX2 instructions and if the OS has enabled them.
 * @return true if AVX2 is available, false otherwise.
 */
#if !defined(_MSC_VER)
__attribute__((target("xsave")))
#endif
inline bool
checkIfAVX2Available()
{
	int cpuInfo[4];
	auto cpuid = [&cpuInfo](int leaf, int subleaf = 0) {
#if defined(_MSC_VER)
		__cpuidex(cpuInfo, leaf, subleaf);
#else
		__cpuid_count(leaf, subleaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
	};

	// 1. Check for AVX support
	// Leaf 1, ECX[28] = AVX, ECX[27] = OSXSAVE
	cpuid(1, 0);
	bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
	bool avx = (cpuInfo[2] & (1 << 28)) != 0;

	if (!avx || !osxsave)
		return false;

	// 2. Check if OS saves YMM registers (XCR0)
	// _xgetbv(0) returns the XCR0 register
	// Bit 1 = SSE state, Bit 2 = AVX state. Both must be 1.
	unsigned long long xcr_val = _xgetbv(0);
	if ((xcr_val & 0x6) != 0x6)
		return false;

	// 3. Check for AVX2 support
	// Leaf 7, Subleaf 0, EBX[5] = AVX2
	cpuid(7, 0);
	return (cpuInfo[1] & (1 << 5)) != 0;
}

inline bool calculateBoundingBoxAVX2256x144(BoundingBox *boundingBox, const std::uint8_t *data, std::uint8_t threshold)
{
	constexpr std::uint32_t width = 256;
	constexpr std::uint32_t height = 144;
	constexpr std::int32_t num_blocks = 8;

	const __m256i v_offset = _mm256_set1_epi8(-128);
	const __m256i v_threshold = _mm256_set1_epi8(static_cast<std::int8_t>(threshold - 128));

	__m256i col_acc[num_blocks];
	for (int i = 0; i < num_blocks; ++i) {
		col_acc[i] = _mm256_setzero_si256();
	}

	int row_flags[height];

	for (std::uint32_t y = 0; y < height; ++y) {
		const __m256i *row_ptr = reinterpret_cast<const __m256i *>(data + y * width);
		__m256i row_any = _mm256_setzero_si256();

		for (std::int32_t b = 0; b < num_blocks; ++b) {
			__m256i v_data = _mm256_loadu_si256(&row_ptr[b]);
			v_data = _mm256_add_epi8(v_data, v_offset);
			__m256i v_cmp = _mm256_cmpgt_epi8(v_data, v_threshold);

			col_acc[b] = _mm256_or_si256(col_acc[b], v_cmp);
			row_any = _mm256_or_si256(row_any, v_cmp);
		}

		row_flags[y] = _mm256_movemask_epi8(row_any);
	}

	std::int32_t min_y = -1;
	std::int32_t max_y = -1;

	for (std::uint32_t y = 0; y < height; ++y) {
		if (row_flags[y] != 0) {
			min_y = static_cast<std::int32_t>(y);
			break;
		}
	}

	if (min_y == -1)
		return false;

	for (std::int32_t y = static_cast<std::int32_t>(height) - 1; y >= min_y; --y) {
		if (row_flags[y] != 0) {
			max_y = y;
			break;
		}
	}

	std::int32_t min_x = width;
	std::int32_t max_x = -1;

	for (std::int32_t b = 0; b < num_blocks; ++b) {
		int mask = _mm256_movemask_epi8(col_acc[b]);

		if (mask != 0) {
			min_x = (b * 32) + get_first_bit_index(mask);
			break;
		}
	}

	for (std::int32_t b = num_blocks - 1; b >= 0; --b) {
		int mask = _mm256_movemask_epi8(col_acc[b]);

		if (mask != 0) {
			max_x = (b * 32) + get_last_bit_index(mask);
			break;
		}
	}

	if (max_x < min_x)
		return false;

	boundingBox->x = min_x;
	boundingBox->y = min_y;
	boundingBox->width = max_x - min_x + 1;
	boundingBox->height = max_y - min_y + 1;

	return true;
}
#endif // __AVX2__

inline bool calculateBoundingBox(BoundingBox *boundingBox, const std::uint8_t *data, std::uint32_t width,
				 std::uint32_t height, std::uint8_t threshold)
{
	std::int32_t min_x = static_cast<std::int32_t>(width);
	std::int32_t max_x = -1;
	std::int32_t min_y = static_cast<std::int32_t>(height);
	std::int32_t max_y = -1;

	for (std::uint32_t y = 0; y < height; ++y) {
		const std::uint8_t *row_ptr = data + (y * width);

		for (std::uint32_t x = 0; x < width; ++x) {
			std::uint8_t pixel = row_ptr[x];

			if (pixel > threshold) {
				if (static_cast<std::int32_t>(x) < min_x)
					min_x = x;
				if (static_cast<std::int32_t>(x) > max_x)
					max_x = x;

				if (static_cast<std::int32_t>(y) < min_y)
					min_y = y;
				if (static_cast<std::int32_t>(y) > max_y)
					max_y = y;
			}
		}
	}

	if (max_y == -1)
		return false;

	boundingBox->x = min_x;
	boundingBox->y = min_y;
	boundingBox->width = max_x - min_x + 1;
	boundingBox->height = max_y - min_y + 1;

	return true;
}

} // anonymous namespace

bool BoundingBox::calculateBoundingBoxFrom256x144(const std::uint8_t *data, std::uint8_t threshold)
{
#if defined(__ARM_NEON)
	return calculateBoundingBoxNEON256x144(this, data, threshold);
#elif defined(__AVX2__)
	const static bool isAVX2Available = checkIfAVX2Available();
	if (isAVX2Available) {
		return calculateBoundingBoxAVX2256x144(this, data, threshold);
	} else {
		return calculateBoundingBox(this, data, 256, 144, threshold);
	}
#else
	return calculateBoundingBox(this, data, 256, 144, threshold);
#endif
}

} // namespace KaitoTokyo::SelfieSegmenter
