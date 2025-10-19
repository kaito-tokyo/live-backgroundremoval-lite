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

#include "NcnnSelfieSegmenter.hpp"

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
namespace LiveBackgroundRemovalLite {

namespace {

inline void copyDataToMatNaive(ncnn::Mat &inputMat, const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	float *rChannel = inputMat.channel(0);
	float *gChannel = inputMat.channel(1);
	float *bChannel = inputMat.channel(2);

	for (std::size_t i = 0; i < pixelCount; i++) {
		bChannel[i] = (static_cast<float>(bgraData[i * 4 + 0])) / 255.0f;
		gChannel[i] = (static_cast<float>(bgraData[i * 4 + 1])) / 255.0f;
		rChannel[i] = (static_cast<float>(bgraData[i * 4 + 2])) / 255.0f;
	}
}

#ifdef SELFIE_SEGMENTER_HAVE_NEON

inline void copyDataToMatNeon(ncnn::Mat &inputMat, const std::uint8_t *bgraData, const std::size_t pixelCount)
{
	const float norm_factor = 1.0f / 255.0f;
	const float32x4_t v_norm = vdupq_n_f32(norm_factor);

	float *rChannel = inputMat.channel(0);
	float *gChannel = inputMat.channel(1);
	float *bChannel = inputMat.channel(2);

	const std::size_t neon_limit = (pixelCount / 8) * 8;
	std::size_t i = 0;
	for (; i < neon_limit; i += 8) {
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
copyDataToMatAVX2(ncnn::Mat &inputMat, const std::uint8_t *bgra_data, const std::size_t pixelCount)
{
	float *rChannel = inputMat.channel(0);
	float *gChannel = inputMat.channel(1);
	float *bChannel = inputMat.channel(2);

	constexpr std::size_t PIXELS_PER_LOOP = 8;

	const std::size_t avx_limit = (pixelCount / PIXELS_PER_LOOP) * PIXELS_PER_LOOP;

	const float norm_factor = 1.0f / 255.0f;
	const __m256 v_inv_255 = _mm256_set1_ps(norm_factor);
	const __m256i mask_u8 = _mm256_set1_epi32(0x000000FF);

	std::size_t i = 0;
	for (; i < avx_limit; i += PIXELS_PER_LOOP) {
		const std::size_t data_offset = i * 4;

		__m256i bgra_u32 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(bgra_data + data_offset));
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
		const std::uint8_t *pixelPtr = bgra_data + i * 4;

		bChannel[i] = static_cast<float>(pixelPtr[0]) * norm_factor;
		gChannel[i] = static_cast<float>(pixelPtr[1]) * norm_factor;
		rChannel[i] = static_cast<float>(pixelPtr[2]) * norm_factor;
	}
}

#else

inline bool checkIfAVX2Available()
{
	return false;
}

#endif // SELFIE_SEGMENTER_CHECK_AVX2

} // namespace

void NcnnSelfieSegmenter::process(const std::uint8_t *bgraData)
{
	if (!bgraData) {
		throw std::invalid_argument("bgraData is null");
	}

#if defined(SELFIE_SEGMENTER_HAVE_NEON)
	copyDataToMatNeon(inputMat, bgraData, getPixelCount());
#elif defined(SELFIE_SEGMENTER_CHECK_AVX2)
	if (isAVX2Available) {
		copyDataToMatAVX2(inputMat, bgraData, getPixelCount());
	} else {
		copyDataToMatNaive(inputMat, bgraData, getPixelCount());
	}
#else
	copyDataToMatNaive(inputMat, bgraData, getPixelCount());
#endif

	ncnn::Extractor ex = selfieSegmenterNet.create_extractor();
	ex.input("in0", inputMat);
	ex.extract("out0", outputMat);

	const float *srcPtr = outputMat.channel(0);
	maskBuffer.write([this, srcPtr](std::vector<std::uint8_t> &mask) {
		for (std::size_t i = 0; i < getPixelCount(); i++) {
			mask[i] = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, srcPtr[i] * 255.f)));
		}
	});
}

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
