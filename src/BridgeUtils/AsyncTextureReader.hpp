/*
Bridge Utils
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

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "GsUnique.hpp"

namespace KaitoTokyo {
namespace BridgeUtils {

namespace AsyncTextureReaderDetail {

inline std::uint32_t getBytesPerPixel(const gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:
		throw std::runtime_error("GS_UNKNOWN format is not supported");
	case GS_A8:
	case GS_R8:
		return 1;
	case GS_R8G8:
		return 2;
	case GS_R16:
	case GS_R16F:
		return 2;
	case GS_RGBA:
	case GS_BGRA:
	case GS_BGRX:
	case GS_R10G10B10A2:
	case GS_R32F:
	case GS_RGBA_UNORM:
	case GS_BGRA_UNORM:
	case GS_BGRX_UNORM:
		return 4;
	case GS_RGBA16:
	case GS_RGBA16F:
		return 8;
	case GS_RGBA32F:
		return 16;
	case GS_RG16:
	case GS_RG16F:
		return 4;
	case GS_RG32F:
		return 8;
	case GS_DXT1:
	case GS_DXT3:
	case GS_DXT5:
		throw std::runtime_error("Compressed formats are not supported");
	default:
		throw std::runtime_error("Unsupported color format");
	}
}

class ScopedStageSurfMap {
private:
	struct MappedData {
		std::uint8_t *data;
		std::uint32_t linesize;
	};

	gs_stagesurf_t *const surf;
	const MappedData mappedData;

	static MappedData map(gs_stagesurf_t *surf)
	{
		if (!surf) {
			throw std::invalid_argument("Target surface cannot be null.");
		}

		MappedData mappedData;
		if (!gs_stagesurface_map(surf, &mappedData.data, &mappedData.linesize)) {
			throw std::runtime_error("gs_stagesurface_map failed");
		}

		if (mappedData.data == nullptr) {
			throw std::runtime_error("gs_stagesurface_map returned null data");
		}

		return mappedData;
	}

public:
	explicit ScopedStageSurfMap(gs_stagesurf_t *_surf) : surf{_surf}, mappedData{map(surf)} {}

	~ScopedStageSurfMap() noexcept
	{
		if (surf) {
			gs_stagesurface_unmap(surf);
		}
	}

	ScopedStageSurfMap(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap &operator=(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap(ScopedStageSurfMap &&) = delete;
	ScopedStageSurfMap &operator=(ScopedStageSurfMap &&) = delete;

	const std::uint8_t *getData() const noexcept { return mappedData.data; }

	std::uint32_t getLinesize() const noexcept { return mappedData.linesize; }
};

} // namespace AsyncTextureReaderDetail

/**
 * @class AsyncTextureReader
 * @brief A double-buffering pipeline for asynchronously reading GPU textures to the CPU.
 *
 * Efficiently copies GPU texture contents to CPU memory without blocking the render thread.
 * Provides thread-safe data access by calling stage() from a render/GPU thread
 * and sync()/getBuffer() from a CPU thread.
 */
class AsyncTextureReader {
public:
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t bufferLinesize;

private:
	std::array<std::vector<std::uint8_t>, 2> cpuBuffers;
	std::atomic<std::size_t> activeCpuBufferIndex = {0};

	std::array<BridgeUtils::unique_gs_stagesurf_t, 2> stagesurfs;
	std::size_t gpuWriteIndex = 0;
	std::mutex gpuMutex;

public:
	/**
    * @brief Constructs the AsyncTextureReader and allocates all necessary resources.
    * @param width The width of the textures to be read.
    * @param height The height of the textures to be read.
    * @param format The color format of the textures.
    */
	AsyncTextureReader(const std::uint32_t _width, const std::uint32_t _height, const gs_color_format format)
		: width(_width),
		  height(_height),
		  bufferLinesize(_width * AsyncTextureReaderDetail::getBytesPerPixel(format)),
		  cpuBuffers{std::vector<std::uint8_t>(static_cast<std::size_t>(height) * bufferLinesize),
			     std::vector<std::uint8_t>(static_cast<std::size_t>(height) * bufferLinesize)},
		  stagesurfs{BridgeUtils::make_unique_gs_stagesurf(width, height, format),
			     BridgeUtils::make_unique_gs_stagesurf(width, height, format)}
	{
	}

	/**
   * @brief Schedules a GPU texture copy. Call from the render/GPU thread.
   * @param sourceTexture The source GPU texture to copy.
   */
	void stage(const unique_gs_texture_t &sourceTexture) noexcept
	{
		std::lock_guard<std::mutex> lock(gpuMutex);
		gs_stage_texture(stagesurfs[gpuWriteIndex].get(), sourceTexture.get());
		gpuWriteIndex = 1 - gpuWriteIndex;
	}

	/**
   * @brief Synchronizes the latest texture data to the CPU buffer. Call from a CPU thread.
   *
   * This is a potentially expensive operation due to GPU-to-CPU data transfer.
   * @throws std::runtime_error If mapping the staging surface fails.
   */
	void sync()
	{
		using namespace AsyncTextureReaderDetail;

		std::size_t gpuReadIndex;
		{
			std::lock_guard<std::mutex> lock(gpuMutex);
			gpuReadIndex = 1 - gpuWriteIndex;
		}
		gs_stagesurf_t *const stagesurf = stagesurfs[gpuReadIndex].get();

		const ScopedStageSurfMap mappedSurf(stagesurf);

		const std::size_t backBufferIndex = 1 - activeCpuBufferIndex.load(std::memory_order_acquire);
		auto &backBuffer = cpuBuffers[backBufferIndex];

		if (bufferLinesize == mappedSurf.getLinesize()) {
			std::memcpy(backBuffer.data(), mappedSurf.getData(),
				    static_cast<std::size_t>(height) * bufferLinesize);
		} else {
			for (std::uint32_t y = 0; y < height; y++) {
				const std::uint8_t *srcRow = mappedSurf.getData() + (y * mappedSurf.getLinesize());
				std::uint8_t *dstRow = backBuffer.data() + (y * bufferLinesize);
				std::size_t copyBytes = std::min<std::size_t>(bufferLinesize, mappedSurf.getLinesize());
				std::memcpy(dstRow, srcRow, copyBytes);
			}
		}

		activeCpuBufferIndex.store(backBufferIndex, std::memory_order_release);
	}

	/**
   * @brief Gets a reference to the CPU buffer with the synced pixel data.
   *
   * This operation is lock-free for immediate data access.
   * @return A read-only buffer containing the latest pixel data.
   */
	const std::vector<std::uint8_t> &getBuffer() const noexcept
	{
		return cpuBuffers[activeCpuBufferIndex.load(std::memory_order_acquire)];
	}
};

} // namespace BridgeUtils
} // namespace KaitoTokyo
