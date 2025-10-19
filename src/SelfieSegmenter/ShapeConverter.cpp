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

#include "ShapeConverter.hpp"

#include <algorithm>

#if defined(_M_X64) || defined(__x86_64__)
#define SELFIE_SEGMENTER_CHECK_AVX2
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#include <x86intrin.h>
#endif // defined(_MSC_VER)
#endif // defined(_M_X64) || defined(__x86_64__)

#if defined(_M_ARM64) || defined(__aarch64__)
#ifdef __ARM_NEON
#define SELFIE_SEGMENTER_HAVE_NEON
#include <arm_neon.h>
#endif // __ARM_NEON
#endif // defined(_M_ARM64) || defined(__aarch64__)

namespace KaitoTokyo {
namespace SelfieSegmenter {

namespace {

inline void copy_r8_bgra_to_float_chw_naive(float *rChannel, float *gChannel, float *bChannel,
					    const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	for (std::size_t i = 0; i < pixelCount; i++) {
		bChannel[i] = (static_cast<float>(bgraData[i * 4 + 0])) / 255.0f;
		gChannel[i] = (static_cast<float>(bgraData[i * 4 + 1])) / 255.0f;
		rChannel[i] = (static_cast<float>(bgraData[i * 4 + 2])) / 255.0f;
	}
}

inline void copy_float_to_clamped_r8_naive(std::uint8_t *dstPtr, float *srcPtr, std::size_t pixelCount)
{
	for (std::size_t i = 0; i < pixelCount; i++) {
		dstPtr[i] = static_cast<std::uint8_t>(std::clamp(srcPtr[i] * 255.f, 0.f, 255.f));
	}
}

#ifdef SELFIE_SEGMENTER_HAVE_NEON

inline void copy_r8_bgra_to_float_chw_neon(float *rChannel, float *gChannel, float *bChannel,
					   const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	constexpr std::size_t PIXELS_PER_LOOP = 8;

	constexpr float norm_factor = 1.0f / 255.0f;
	const float32x4_t v_norm = vdupq_n_f32(norm_factor);

	const std::size_t neon_limit = (pixelCount / PIXELS_PER_LOOP) * PIXELS_PER_LOOP;
	std::size_t i = 0;
	for (; i < neon_limit; i += PIXELS_PER_LOOP) {
		uint8x8x4_t bgra_vec = vld4_u8(bgraData + i * 4);

		uint16x8_t b_u16 = vmovl_u8(bgra_vec.val[0]);
		uint16x8_t g_u16 = vmovl_u8(bgra_vec.val[1]);
		uint16x8_t r_u16 = vmovl_u8(bgra_vec.val[2]);

		float32x4_t b_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b_u16))), v_norm);
		float32x4_t g_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g_u16))), v_norm);
		float32x4_t r_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r_u16))), v_norm);

		float32x4_t b_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b_u16))), v_norm);
		float32x4_t g_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g_u16))), v_norm);
		float32x4_t r_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r_u16))), v_norm);

		vst1q_f32(bChannel + i, b_f32_low);
		vst1q_f32(bChannel + i + 4, b_f32_high);
		vst1q_f32(gChannel + i, g_f32_low);
		vst1q_f32(gChannel + i + 4, g_f32_high);
		vst1q_f32(rChannel + i, r_f32_low);
		vst1q_f32(rChannel + i + 4, r_f32_high);
	}

	for (; i < pixelCount; ++i) {
		const std::uint8_t *pixelPtr = bgraData + i * 4;

		bChannel[i] = static_cast<float>(pixelPtr[0]) * norm_factor;
		gChannel[i] = static_cast<float>(pixelPtr[1]) * norm_factor;
		rChannel[i] = static_cast<float>(pixelPtr[2]) * norm_factor;
	}
}

inline void copy_float32_to_r8_neon(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
	// Process 32 pixels at a time to maximize ILP on Apple cores
	constexpr std::size_t FLOATS_PER_LOOP = 32;

	constexpr float scale_factor = 255.0f;

	std::size_t neon_limit = (pixel_count / FLOATS_PER_LOOP) * FLOATS_PER_LOOP;
	std::size_t i = 0;

	for (; i < neon_limit; i += FLOATS_PER_LOOP) {
		// --- Block 1: Pixels 0-15 ---

		// 1.1 Load 16 floats (Block 1)
		float32x4_t v_f32_1 = vld1q_f32(src + i + 0);
		float32x4_t v_f32_2 = vld1q_f32(src + i + 4);
		float32x4_t v_f32_3 = vld1q_f32(src + i + 8);
		float32x4_t v_f32_4 = vld1q_f32(src + i + 12);

		// --- Block 2: Pixels 16-31 ---

		// 1.2 Load 16 floats (Block 2)
		float32x4_t v_f32_5 = vld1q_f32(src + i + 16);
		float32x4_t v_f32_6 = vld1q_f32(src + i + 20);
		float32x4_t v_f32_7 = vld1q_f32(src + i + 24);
		float32x4_t v_f32_8 = vld1q_f32(src + i + 28);

		// 2.1 Scale (Block 1)
		v_f32_1 = vmulq_n_f32(v_f32_1, scale_factor);
		v_f32_2 = vmulq_n_f32(v_f32_2, scale_factor);
		v_f32_3 = vmulq_n_f32(v_f32_3, scale_factor);
		v_f32_4 = vmulq_n_f32(v_f32_4, scale_factor);

		// 2.2 Scale (Block 2)
		v_f32_5 = vmulq_n_f32(v_f32_5, scale_factor);
		v_f32_6 = vmulq_n_f32(v_f32_6, scale_factor);
		v_f32_7 = vmulq_n_f32(v_f32_7, scale_factor);
		v_f32_8 = vmulq_n_f32(v_f32_8, scale_factor);

		// 3.1 Convert f32 -> u32 (Block 1)
		uint32x4_t v_u32_1 = vcvtq_u32_f32(v_f32_1);
		uint32x4_t v_u32_2 = vcvtq_u32_f32(v_f32_2);
		uint32x4_t v_u32_3 = vcvtq_u32_f32(v_f32_3);
		uint32x4_t v_u32_4 = vcvtq_u32_f32(v_f32_4);

		// 3.2 Convert f32 -> u32 (Block 2)
		uint32x4_t v_u32_5 = vcvtq_u32_f32(v_f32_5);
		uint32x4_t v_u32_6 = vcvtq_u32_f32(v_f32_6);
		uint32x4_t v_u32_7 = vcvtq_u32_f32(v_f32_7);
		uint32x4_t v_u32_8 = vcvtq_u32_f32(v_f32_8);

		// 4.1 Narrow u32 -> u16 (Block 1)
		uint16x4_t v_u16_1 = vmovn_u32(v_u32_1);
		uint16x4_t v_u16_2 = vmovn_u32(v_u32_2);
		uint16x4_t v_u16_3 = vmovn_u32(v_u32_3);
		uint16x4_t v_u16_4 = vmovn_u32(v_u32_4);

		// 4.2 Narrow u32 -> u16 (Block 2)
		uint16x4_t v_u16_5 = vmovn_u32(v_u32_5);
		uint16x4_t v_u16_6 = vmovn_u32(v_u32_6);
		uint16x4_t v_u16_7 = vmovn_u32(v_u32_7);
		uint16x4_t v_u16_8 = vmovn_u32(v_u32_8);

		// 5.1 Combine & Narrow u16 -> u8 (Block 1)
		uint16x8_t v_u16_lo_1 = vcombine_u16(v_u16_1, v_u16_2);
		uint16x8_t v_u16_hi_1 = vcombine_u16(v_u16_3, v_u16_4);
		uint8x8_t v_u8_lo_1 = vmovn_u16(v_u16_lo_1);
		uint8x8_t v_u8_hi_1 = vmovn_u16(v_u16_hi_1);

		// 5.2 Combine & Narrow u16 -> u8 (Block 2)
		uint16x8_t v_u16_lo_2 = vcombine_u16(v_u16_5, v_u16_6);
		uint16x8_t v_u16_hi_2 = vcombine_u16(v_u16_7, v_u16_8);
		uint8x8_t v_u8_lo_2 = vmovn_u16(v_u16_lo_2);
		uint8x8_t v_u8_hi_2 = vmovn_u16(v_u16_hi_2);

		// 6.1 Combine & Store (Block 1)
		uint8x16_t v_out_1 = vcombine_u8(v_u8_lo_1, v_u8_hi_1);
		vst1q_u8(dst + i, v_out_1);

		// 6.2 Combine & Store (Block 2)
		uint8x16_t v_out_2 = vcombine_u8(v_u8_lo_2, v_u8_hi_2);
		vst1q_u8(dst + i + 16, v_out_2);
	}

	// Handle remaining elements (scalar)
	for (; i < pixel_count; ++i) {
		// No rounding or clamping, rely on standard C++ (float -> u8) truncation
		dst[i] = static_cast<std::uint8_t>(src[i] * scale_factor);
	}
}

#endif // SELFIE_SEGMENTER_HAVE_NEON

#ifdef SELFIE_SEGMENTER_CHECK_AVX2

#if !defined(_MSC_VER)
__attribute__((target("xsave")))
#endif
inline bool
check_if_avx2_available()
{
	int cpuInfo[4];
	auto cpuid = [&](int leaf, int subleaf = 0) {
#if defined(_MSC_VER)
		__cpuidex(cpuInfo, leaf, subleaf);
#else
		__cpuid_count(leaf, subleaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
	};

	cpuid(1);
	bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
	if (!osxsave)
		return false;

	unsigned long long xcr_val = _xgetbv(0);
	if ((xcr_val & 0x6) != 0x6)
		return false;

	cpuid(7, 0);
	return (cpuInfo[1] & (1 << 5)) != 0;
}

#if !defined(_MSC_VER)
__attribute__((target("avx,avx2")))
#endif
inline void
copy_r8_bgra_to_float_chw_avx2(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgraData,
			       const std::size_t pixelCount)
{
	constexpr std::size_t PIXELS_PER_LOOP = 8;

	const std::size_t avx_limit = (pixelCount / PIXELS_PER_LOOP) * PIXELS_PER_LOOP;

	constexpr float norm_factor = 1.0f / 255.0f;
	const __m256 v_inv_255 = _mm256_set1_ps(norm_factor);
	const __m256i mask_u8 = _mm256_set1_epi32(0x000000FF);

	std::size_t i = 0;
	for (; i < avx_limit; i += PIXELS_PER_LOOP) {
		const std::size_t data_offset = i * 4;

		__m256i bgra_u32 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(bgraData + data_offset));
		__m256i b_u32 = _mm256_and_si256(bgra_u32, mask_u8);
		__m256i g_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 8), mask_u8);
		__m256i r_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 16), mask_u8);

		__m256 b_ps = _mm256_cvtepi32_ps(b_u32);
		__m256 g_ps = _mm256_cvtepi32_ps(g_u32);
		__m256 r_ps = _mm256_cvtepi32_ps(r_u32);

		b_ps = _mm256_mul_ps(b_ps, v_inv_255);
		g_ps = _mm256_mul_ps(g_ps, v_inv_255);
		r_ps = _mm256_mul_ps(r_ps, v_inv_255);

		_mm256_storeu_ps(bChannel + i, b_ps);
		_mm256_storeu_ps(gChannel + i, g_ps);
		_mm256_storeu_ps(rChannel + i, r_ps);
	}

	for (; i < pixelCount; ++i) {
		const std::uint8_t *pixelPtr = bgraData + i * 4;

		bChannel[i] = static_cast<float>(pixelPtr[0]) * norm_factor;
		gChannel[i] = static_cast<float>(pixelPtr[1]) * norm_factor;
		rChannel[i] = static_cast<float>(pixelPtr[2]) * norm_factor;
	}
}

#endif // SELFIE_SEGMENTER_CHECK_AVX2

} // namespace

void copy_r8_bgra_to_float_chw(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgraData,
			       const std::size_t pixelCount)
{
#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copy_r8_bgra_to_float_chw_neon(rChannel, gChannel, bChannel, bgraData, pixelCount);
#elif defined(SELFIE_SEGMENTER_CHECK_AVX2)
	static const bool is_avx2_available = check_if_avx2_available();
	if (is_avx2_available) {
		copy_r8_bgra_to_float_chw_avx2(rChannel, gChannel, bChannel, bgraData, pixelCount);
	} else {
		copy_r8_bgra_to_float_chw_naive(rChannel, gChannel, bChannel, bgraData, pixelCount);
	}
#else
	copy_r8_bgra_to_float_chw_naive(rChannel, gChannel, bChannel, bgraData, pixelCount);
#endif
}

inline void copy_float32_to_r8(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copy_float32_to_r8_neon(dst, src, pixel_count);
#else
	copy_float32_to_r8_naive(dst, src, pixel_count);
#endif
}

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
