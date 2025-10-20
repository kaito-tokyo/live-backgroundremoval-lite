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

#include <cassert>
#include <cstring>

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

/**
 * @brief Naive C++ implementation for BGRA (uint8_t) to planar float (CHW) conversion.
 * @param rChannel Pointer to the R channel output (float).
 * @param gChannel Pointer to the G channel output (float).
 * @param bChannel Pointer to the B channel output (float).
 * @param bgraData Pointer to the input BGRA data (uint8_t).
 * @param pixelCount Total number of pixels to process.
 */
inline void copy_r8_bgra_to_float_chw_naive(float *rChannel, float *gChannel, float *bChannel,
					    const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	for (std::size_t i = 0; i < pixelCount; i++) {
		bChannel[i] = (static_cast<float>(bgraData[i * 4 + 0])) / 255.0f;
		gChannel[i] = (static_cast<float>(bgraData[i * 4 + 1])) / 255.0f;
		rChannel[i] = (static_cast<float>(bgraData[i * 4 + 2])) / 255.0f;
	}
}

/**
 * @brief Naive C++ implementation for float (0.0f-1.0f) to uint8_t (0-255) conversion.
 * @param dst Pointer to the output uint8_t buffer.
 * @param src Pointer to the input float buffer.
 * @param pixel_count Total number of pixels to process.
 */
inline void copy_float32_to_r8_naive(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
	for (std::size_t i = 0; i < pixel_count; i++) {
		dst[i] = static_cast<std::uint8_t>(src[i] * 255.f);
	}
}

#ifdef SELFIE_SEGMENTER_HAVE_NEON

/**
 * @brief NEON optimized implementation for BGRA (uint8_t) to planar float (CHW) conversion.
 * @param rChannel Pointer to the R channel output (float). Must be 16-byte aligned.
 * @param gChannel Pointer to the G channel output (float). Must be 16-byte aligned.
 * @param bChannel Pointer to the B channel output (float). Must be 16-byte aligned.
 * @param bgraData Pointer to the input BGRA data (uint8_t). Must be 16-byte aligned.
 * @param pixelCount Total number of pixels to process.
 */
inline void copy_r8_bgra_to_float_chw_neon(float *rChannel, float *gChannel, float *bChannel,
					   const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	assert(reinterpret_cast<std::uintptr_t>(rChannel) % 16 == 0);
	assert(reinterpret_cast<std::uintptr_t>(gChannel) % 16 == 0);
	assert(reinterpret_cast<std::uintptr_t>(bChannel) % 16 == 0);
	assert(reinterpret_cast<std::uintptr_t>(bgraData) % 16 == 0);

	// Process 16 pixels at a time to maximize ILP
	constexpr std::size_t PIXELS_PER_LOOP = 16;

	constexpr float norm_factor = 1.0f / 255.0f;

	std::size_t neon_limit_16x = (pixelCount / PIXELS_PER_LOOP) * PIXELS_PER_LOOP;
	std::size_t i = 0;

	// --- Main 16-pixel loop ---
	for (; i < neon_limit_16x; i += PIXELS_PER_LOOP) {
		// Load 16 pixels (16 * 4 = 64 bytes) and de-interleave
		uint8x16x4_t bgra_vec = vld4q_u8(bgraData + i * 4);

		// --- Block 1: Pixels 0-7 ---
		// 1. Widen u8 -> u16
		uint16x8_t b_u16_1 = vmovl_u8(vget_low_u8(bgra_vec.val[0]));
		uint16x8_t g_u16_1 = vmovl_u8(vget_low_u8(bgra_vec.val[1]));
		uint16x8_t r_u16_1 = vmovl_u8(vget_low_u8(bgra_vec.val[2]));

		// 2. Widen u16 -> u32, Convert u32 -> f32, and Scale
		float32x4_t b_f32_1_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b_u16_1))), norm_factor);
		float32x4_t g_f32_1_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g_u16_1))), norm_factor);
		float32x4_t r_f32_1_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r_u16_1))), norm_factor);

		float32x4_t b_f32_1_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b_u16_1))), norm_factor);
		float32x4_t g_f32_1_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g_u16_1))), norm_factor);
		float32x4_t r_f32_1_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r_u16_1))), norm_factor);

		// --- Block 2: Pixels 8-15 ---
		// 1. Widen u8 -> u16
		uint16x8_t b_u16_2 = vmovl_u8(vget_high_u8(bgra_vec.val[0]));
		uint16x8_t g_u16_2 = vmovl_u8(vget_high_u8(bgra_vec.val[1]));
		uint16x8_t r_u16_2 = vmovl_u8(vget_high_u8(bgra_vec.val[2]));

		// 2. Widen u16 -> u32, Convert u32 -> f32, and Scale
		float32x4_t b_f32_2_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b_u16_2))), norm_factor);
		float32x4_t g_f32_2_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g_u16_2))), norm_factor);
		float32x4_t r_f32_2_low = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r_u16_2))), norm_factor);

		float32x4_t b_f32_2_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b_u16_2))), norm_factor);
		float32x4_t g_f32_2_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g_u16_2))), norm_factor);
		float32x4_t r_f32_2_high = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r_u16_2))), norm_factor);

		// --- Store (12 vectors) ---
		vst1q_f32(bChannel + i, b_f32_1_low);
		vst1q_f32(bChannel + i + 4, b_f32_1_high);
		vst1q_f32(gChannel + i, g_f32_1_low);
		vst1q_f32(gChannel + i + 4, g_f32_1_high);
		vst1q_f32(rChannel + i, r_f32_1_low);
		vst1q_f32(rChannel + i + 4, r_f32_1_high);

		vst1q_f32(bChannel + i + 8, b_f32_2_low);
		vst1q_f32(bChannel + i + 12, b_f32_2_high);
		vst1q_f32(gChannel + i + 8, g_f32_2_low);
		vst1q_f32(gChannel + i + 12, g_f32_2_high);
		vst1q_f32(rChannel + i + 8, r_f32_2_low);
		vst1q_f32(rChannel + i + 12, r_f32_2_high);
	}

	// --- Final Scalar Loop (0-15 pixels) ---
	for (; i < pixelCount; ++i) {
		const std::uint8_t *pixelPtr = bgraData + i * 4;
		bChannel[i] = static_cast<float>(pixelPtr[0]) * norm_factor;
		gChannel[i] = static_cast<float>(pixelPtr[1]) * norm_factor;
		rChannel[i] = static_cast<float>(pixelPtr[2]) * norm_factor;
	}
}

/**
 * @brief NEON optimized implementation for float (0.0f-1.0f) to uint8_t (0-255) conversion.
 * @param dst Pointer to the output uint8_t buffer. Must be 16-byte aligned.
 * @param src Pointer to the input float buffer. Must be 16-byte aligned.
 * @param pixel_count Total number of pixels to process.
 */
inline void copy_float32_to_r8_neon(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
	assert(reinterpret_cast<std::uintptr_t>(dst) % 16 == 0);
	assert(reinterpret_cast<std::uintptr_t>(src) % 16 == 0);

	constexpr std::size_t FLOATS_PER_LOOP = 32;

	constexpr float scale_factor = 255.0f;

	std::size_t i = 0;

	std::size_t neon_limit = (pixel_count / FLOATS_PER_LOOP) * FLOATS_PER_LOOP;
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

	// --- Remainder Loop (Naive C++) ---
	// Handle remaining 0-31 pixels
	for (; i < pixel_count; ++i) {
		dst[i] = static_cast<std::uint8_t>(src[i] * 255.f);
	}
}

#endif // SELFIE_SEGMENTER_HAVE_NEON

#ifdef SELFIE_SEGMENTER_CHECK_AVX2

/**
 * @brief Checks if the CPU supports AVX2 instructions and if the OS has enabled them.
 * @return true if AVX2 is available, false otherwise.
 */
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

	// 1. Check for AVX support
	cpuid(1);
	bool avx = (cpuInfo[2] & (1 << 28)) != 0;
	bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
	if (!avx || !osxsave)
		return false;

	// 2. Check if OS saves YMM registers
	unsigned long long xcr_val = _xgetbv(0);
	if ((xcr_val & 0x6) != 0x6) // Check for SSE (bit 1) and AVX (bit 2) state
		return false;

	// 3. Check for AVX2 support
	cpuid(7, 0);
	return (cpuInfo[1] & (1 << 5)) != 0; // Check EBX[5] for AVX2
}

/**
 * @brief AVX2 optimized implementation for BGRA (uint8_t) to planar float (CHW) conversion.
 * Uses an efficient bitwise deinterleaving strategy.
 * @param rChannel Pointer to the R channel output (float). Must be 32-byte aligned.
 * @param gChannel Pointer to the G channel output (float). Must be 32-byte aligned.
 * @param bChannel Pointer to the B channel output (float). Must be 32-byte aligned.
 * @param bgraData Pointer to the input BGRA data (uint8_t). Must be 32-byte aligned.
 * @param pixelCount Total number of pixels to process.
 */
#if !defined(_MSC_VER)
__attribute__((target("avx,avx2")))
#endif
inline void
copy_r8_bgra_to_float_chw_avx2(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgraData,
			       const std::size_t pixelCount)
{
	// --- 0. Pre-condition checks ---
	// Assert that input/output pointers are 32-byte aligned
	assert(reinterpret_cast<std::uintptr_t>(rChannel) % 32 == 0);
	assert(reinterpret_cast<std::uintptr_t>(gChannel) % 32 == 0);
	assert(reinterpret_cast<std::uintptr_t>(bChannel) % 32 == 0);
	assert(reinterpret_cast<std::uintptr_t>(bgraData) % 32 == 0);

	constexpr std::size_t PIXELS_PER_LOOP = 8;

	// Calculate the boundary for the main AVX2 loop (8 pixels per iteration)
	const std::size_t avx_limit = (pixelCount / PIXELS_PER_LOOP) * PIXELS_PER_LOOP;

	// --- 1. Prepare constant registers ---
	// Normalization factor (1.0f / 255.0f) broadcasted to 8 floats
	constexpr float norm_factor = 1.0f / 255.0f;
	const __m256 v_inv_255 = _mm256_set1_ps(norm_factor);
	// Bitmask to extract the lower 8 bits of each 32-bit integer (for uint8_t -> int32_t)
	const __m256i mask_u8 = _mm256_set1_epi32(0x000000FF);

	std::size_t i = 0;
	// --- 2. Main loop (8 pixels per iteration) ---
	for (; i < avx_limit; i += PIXELS_PER_LOOP) {
		const std::size_t data_offset = i * 4;

		// Step 2a: Load 8 pixels (32 bytes) (Aligned)
		// Loads [B0 G0 R0 A0] ... [B7 G7 R7 A7] as 8x int32_t
		__m256i bgra_u32 = _mm256_load_si256(reinterpret_cast<const __m256i *>(bgraData + data_offset));

		// Step 2b: Channel separation (deinterleave)
		// AND with bitmask (0xFF) to extract B channel (lower 8 bits)
		__m256i b_u32 = _mm256_and_si256(bgra_u32, mask_u8);
		// Right-shift by 8 bits and mask to extract G channel
		__m256i g_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 8), mask_u8);
		// Right-shift by 16 bits and mask to extract R channel
		__m256i r_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 16), mask_u8);

		// Step 2c: Convert int32_t to float
		// Convert [B0..B7] (int32) to [B0..B7] (float)
		__m256 b_ps = _mm256_cvtepi32_ps(b_u32);
		__m256 g_ps = _mm256_cvtepi32_ps(g_u32);
		__m256 r_ps = _mm256_cvtepi32_ps(r_u32);

		// Step 2d: Normalize (0.0 - 1.0)
		// Multiply by the normalization factor (1.0/255.0)
		b_ps = _mm256_mul_ps(b_ps, v_inv_255);
		g_ps = _mm256_mul_ps(g_ps, v_inv_255);
		r_ps = _mm256_mul_ps(r_ps, v_inv_255);

		// Step 2e: Store results (Aligned)
		// Write 8 float values to each channel's memory
		_mm256_store_ps(bChannel + i, b_ps);
		_mm256_store_ps(gChannel + i, g_ps);
		_mm256_store_ps(rChannel + i, r_ps);
	}

	// --- 3. Handle remaining pixels (1-7 pixels) ---
	// Use naive C++ for the remainder
	for (; i < pixelCount; ++i) {
		const std::uint8_t *pixelPtr = bgraData + i * 4;
		bChannel[i] = static_cast<float>(pixelPtr[0]) * norm_factor;
		gChannel[i] = static_cast<float>(pixelPtr[1]) * norm_factor;
		rChannel[i] = static_cast<float>(pixelPtr[2]) * norm_factor;
	}
}

/**
 * Converts an array of floats (0.0f-1.0f) to an array of uint8_t (0-255)
 * using AVX2 with the Pack-Pack-Permute strategy.
 *
 * @param dst Output buffer (uint8_t*). Must be 32-byte aligned.
 * @param src Input buffer (float*). Must be 32-byte aligned.
 * @param pixel_count The number of pixels to process.
 */
#if !defined(_MSC_VER)
__attribute__((target("avx,avx2")))
#endif
inline void
convert_float_to_uint8_avx2(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
	// --- 0. Pre-condition checks (32-byte alignment) ---
	assert(reinterpret_cast<std::uintptr_t>(dst) % 32 == 0);
	assert(reinterpret_cast<std::uintptr_t>(src) % 32 == 0);

	// --- 1. Prepare constant registers ---
	const __m256 v_255 = _mm256_set1_ps(255.0f);

	// Mask to re-order data after the Pack-Pack steps.
	// AVX2 packs shuffle data across lanes. This mask undoes that shuffle.
	// Input order (32-bit lanes): [0, 1, 2, 3 | 4, 5, 6, 7]
	// Desired order:              [0, 4, 1, 5, 2, 6, 3, 7]
	const __m256i permute_mask = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);

	std::size_t i = 0;

	// --- 2. Main loop (32 pixels per iteration) ---
	// 32 floats (128B) -> 32 uint8s (32B)
	const std::size_t avx_limit_32 = (pixel_count / 32) * 32;
	for (; i < avx_limit_32; i += 32) {

		// Step 2a: Load floats, multiply by 255, and convert to int32 (truncate)
		__m256 v_f0 = _mm256_load_ps(src + i + 0);
		__m256 v_f1 = _mm256_load_ps(src + i + 8);
		__m256 v_f2 = _mm256_load_ps(src + i + 16);
		__m256 v_f3 = _mm256_load_ps(src + i + 24);

		v_f0 = _mm256_mul_ps(v_f0, v_255);
		v_f1 = _mm256_mul_ps(v_f1, v_255);
		v_f2 = _mm256_mul_ps(v_f2, v_255);
		v_f3 = _mm256_mul_ps(v_f3, v_255);

		__m256i v0 = _mm256_cvttps_epi32(v_f0); // [i0..i7]
		__m256i v1 = _mm256_cvttps_epi32(v_f1); // [i8..i15]
		__m256i v2 = _mm256_cvttps_epi32(v_f2); // [i16..i23]
		__m256i v3 = _mm256_cvttps_epi32(v_f3); // [i24..i31]

		// Step 2b: Pack (int32 -> int16)
		// Data gets interleaved due to AVX2 lane constraints
		// v01_16 = [i0..3, i8..11 | i4..7, i12..15]
		__m256i v01_16 = _mm256_packs_epi32(v0, v1);
		// v23_16 = [i16..19, i24..27 | i20..23, i28..31]
		__m256i v23_16 = _mm256_packs_epi32(v2, v3);

		// Step 2c: Pack (int16 -> uint8)
		// Data is further interleaved
		// v_interleaved = [i0..3, i8..11, i16..19, i24..27 | i4..7, i12..15, i20..23, i28..31]
		__m256i v_interleaved = _mm256_packus_epi16(v01_16, v23_16);

		// Step 2d: Re-order (Permute)
		// Re-order the 32-bit lanes to restore the correct [i0..31] sequence
		__m256i v_result = _mm256_permutevar8x32_epi32(v_interleaved, permute_mask);

		// Step 2e: Store 32 bytes (32 uint8_t)
		_mm256_store_si256(reinterpret_cast<__m256i *>(dst + i), v_result);
	}

	// --- 3. Remainder loop (1 pixel per iteration) ---
	// Handle remaining 0-31 pixels with naive C++
	for (; i < pixel_count; ++i) {
		dst[i] = static_cast<std::uint8_t>(src[i] * 255.f);
	}
}

#endif // SELFIE_SEGMENTER_CHECK_AVX2

} // namespace

/**
 * @brief Converts BGRA (uint8_t) image data to planar float (CHW) format.
 *
 * This function dispatches to the best available implementation (AVX2, NEON, or naive)
 * at runtime.
 *
 * @param rChannel Pointer to the R channel output (float).
 * @param gChannel Pointer to the G channel output (float).
 * @param bChannel Pointer to the B channel output (float).
 * @param bgraData Pointer to the input BGRA data (uint8_t).
 * @param pixelCount Total number of pixels to process.
 */
void copy_r8_bgra_to_float_chw(float *rChannel, float *gChannel, float *bChannel, const std::uint8_t *bgraData,
			       const std::size_t pixelCount)
{
#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copy_r8_bgra_to_float_chw_neon(rChannel, gChannel, bChannel, bgraData, pixelCount);
#elif defined(SELFIE_SEGMENTER_CHECK_AVX2)
	const static bool is_avx2_available = check_if_avx2_available();
	if (is_avx2_available) {
		copy_r8_bgra_to_float_chw_avx2(rChannel, gChannel, bChannel, bgraData, pixelCount);
	} else {
		copy_r8_bgra_to_float_chw_naive(rChannel, gChannel, bChannel, bgraData, pixelCount);
	}
#else
	copy_r8_bgra_to_float_chw_naive(rChannel, gChannel, bChannel, bgraData, pixelCount);
#endif
}

/**
 * @brief Converts a float (0.0f-1.0f) array to a uint8_t (0-255) array.
 *
 * This function dispatches to the best available implementation (AVX2, NEON, or naive)
 * at runtime.
 *
 * @param dst Pointer to the output uint8_t buffer.
 * @param src Pointer to the input float buffer.
 * @param pixel_count Total number of pixels to process.
 */
void copy_float32_to_r8(std::uint8_t *dst, const float *src, std::size_t pixel_count)
{
#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copy_float32_to_r8_neon(dst, src, pixel_count);
#elif defined(SELFIE_SEGMENTER_CHECK_AVX2)
	const static bool is_avx2_available = check_if_avx2_available();
	if (is_avx2_available) {
		convert_float_to_uint8_avx2(dst, src, pixel_count);
	} else {
		copy_float32_to_r8_naive(dst, src, pixel_count);
	}
#else
	copy_float32_to_r8_naive(dst, src, pixel_count);
#endif
}

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
