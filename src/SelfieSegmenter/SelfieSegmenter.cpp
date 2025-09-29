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

#include "SelfieSegmenter.hpp"

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
namespace BackgroundRemovalLite {

namespace {

inline void copyDataToMatNaive(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	for (int i = 0; i < SelfieSegmenter::PIXEL_COUNT; i++) {
		b_channel[i] = (static_cast<float>(bgra_data[i * 4 + 0])) / 255.0f;
		g_channel[i] = (static_cast<float>(bgra_data[i * 4 + 1])) / 255.0f;
		r_channel[i] = (static_cast<float>(bgra_data[i * 4 + 2])) / 255.0f;
	}
}

#ifdef SELFIE_SEGMENTER_HAVE_NEON

inline void copyDataToMatNeon(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	const float32x4_t v_norm = vdupq_n_f32(1.0f / 255.0f);

	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	for (int i = 0; i < SelfieSegmenter::PIXEL_COUNT; i += 8) {
		uint8x8x4_t bgra_vec = vld4_u8(bgra_data + i * 4);

		uint16x8_t b_u16 = vmovl_u8(bgra_vec.val[0]);
		uint16x8_t g_u16 = vmovl_u8(bgra_vec.val[1]);
		uint16x8_t r_u16 = vmovl_u8(bgra_vec.val[2]);

		float32x4_t b_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b_u16))), v_norm);
		float32x4_t g_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g_u16))), v_norm);
		float32x4_t r_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r_u16))), v_norm);

		float32x4_t b_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b_u16))), v_norm);
		float32x4_t g_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g_u16))), v_norm);
		float32x4_t r_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r_u16))), v_norm);

		vst1q_f32(b_channel + i, b_f32_low);
		vst1q_f32(b_channel + i + 4, b_f32_high);
		vst1q_f32(g_channel + i, g_f32_low);
		vst1q_f32(g_channel + i + 4, g_f32_high);
		vst1q_f32(r_channel + i, r_f32_low);
		vst1q_f32(r_channel + i + 4, r_f32_high);
	}
}

#endif // SELFIE_SEGMENTER_HAVE_NEON

#ifdef SELFIE_SEGMENTER_CHECK_AVX2

#if !defined(_MSC_VER)
__attribute__((target("xsave")))
#endif
inline bool
checkIfAVX2Available()
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
copyDataToMatAVX2(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	constexpr int PIXELS_PER_LOOP = 8;
	const int num_loops = SelfieSegmenter::PIXEL_COUNT / PIXELS_PER_LOOP;

	const __m256 v_inv_255 = _mm256_set1_ps(1.0f / 255.0f);

	const __m128i shuffle_b_mask = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);
	const __m128i shuffle_g_mask = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 13, 9, 5, 1);
	const __m128i shuffle_r_mask = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14, 10, 6, 2);

	for (int i = 0; i < num_loops; ++i) {
		const int offset = i * PIXELS_PER_LOOP;
		const int data_offset = offset * 4;

		__m256i bgra_u8 = _mm256_loadu_si256((__m256i const *)(bgra_data + data_offset));

		__m128i bgra_low = _mm256_castsi256_si128(bgra_u8);
		__m128i bgra_high = _mm256_extracti128_si256(bgra_u8, 1);

		__m128i b_u8_low = _mm_shuffle_epi8(bgra_low, shuffle_b_mask);
		__m128i g_u8_low = _mm_shuffle_epi8(bgra_low, shuffle_g_mask);
		__m128i r_u8_low = _mm_shuffle_epi8(bgra_low, shuffle_r_mask);

		__m128 b_ps_low = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(b_u8_low));
		__m128 g_ps_low = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(g_u8_low));
		__m128 r_ps_low = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(r_u8_low));

		__m128i b_u8_high = _mm_shuffle_epi8(bgra_high, shuffle_b_mask);
		__m128i g_u8_high = _mm_shuffle_epi8(bgra_high, shuffle_g_mask);
		__m128i r_u8_high = _mm_shuffle_epi8(bgra_high, shuffle_r_mask);

		__m128 b_ps_high = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(b_u8_high));
		__m128 g_ps_high = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(g_u8_high));
		__m128 r_ps_high = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(r_u8_high));

		__m256 b_ps = _mm256_insertf128_ps(_mm256_castps128_ps256(b_ps_low), b_ps_high, 1);
		__m256 g_ps = _mm256_insertf128_ps(_mm256_castps128_ps256(g_ps_low), g_ps_high, 1);
		__m256 r_ps = _mm256_insertf128_ps(_mm256_castps128_ps256(r_ps_low), r_ps_high, 1);

		b_ps = _mm256_mul_ps(b_ps, v_inv_255);
		g_ps = _mm256_mul_ps(g_ps, v_inv_255);
		r_ps = _mm256_mul_ps(r_ps, v_inv_255);

		_mm256_storeu_ps(b_channel + offset, b_ps);
		_mm256_storeu_ps(g_channel + offset, g_ps);
		_mm256_storeu_ps(r_channel + offset, r_ps);
	}
}

#else

inline bool checkIfAVX2Available()
{
	return false;
}

#endif // SELFIE_SEGMENTER_CHECK_AVX2

} // namespace

SelfieSegmenter::SelfieSegmenter(const ncnn::Net &_selfieSegmenterNet)
	: selfieSegmenterNet(_selfieSegmenterNet),
	  maskBuffer(PIXEL_COUNT),
	  isAVX2Available(checkIfAVX2Available())
{
	inputMat.create(INPUT_WIDTH, INPUT_HEIGHT, 3, sizeof(float));
}

void SelfieSegmenter::process(const std::uint8_t *bgra_data)
{
	if (!bgra_data) {
		return;
	}

	preprocess(bgra_data);

	ncnn::Extractor ex = selfieSegmenterNet.create_extractor();
	ex.input("in0", inputMat);
	ex.extract("out0", outputMat);

	std::vector<std::uint8_t> &maskToWrite = maskBuffer.beginWrite();
	postprocess(maskToWrite);
	maskBuffer.commitWrite();
}

const std::vector<std::uint8_t> &SelfieSegmenter::getMask() const
{
	return maskBuffer.read();
}

void SelfieSegmenter::preprocess(const std::uint8_t *bgra_data)
{
#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copyDataToMatNeon(inputMat, bgra_data);
#elif defined(SELFIE_SEGMENTER_CHECK_AVX2)
	if (isAVX2Available) {
		copyDataToMatAVX2(inputMat, bgra_data);
	} else {
		copyDataToMatNaive(inputMat, bgra_data);
	}
#else
	copyDataToMatNaive(inputMat, bgra_data);
#endif
}

void SelfieSegmenter::postprocess(std::vector<std::uint8_t> &mask) const
{
	const float *src_ptr = outputMat.channel(0);
	for (int i = 0; i < PIXEL_COUNT; i++) {
		mask[i] = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, src_ptr[i] * 255.f)));
	}
}

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
