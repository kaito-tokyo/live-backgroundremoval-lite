/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * KaitoTokyo ObsBridgeUtils Library
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
namespace ObsBridgeUtils {

/**
 * @brief Internal implementation details for AsyncTextureReader.
 */
namespace AsyncTextureReaderDetail {

/**
 * @class ScopedStageSurfMap
 * @brief RAII helper for mapping and unmapping a gs_stagesurf_t.
 *
 * Ensures that a mapped staging surface is automatically unmapped
 * when this object goes out of scope.
 */
class ScopedStageSurfMap final {
public:
	/**
	 * @brief Maps a staging surface for reading and automatically unmaps on destruction.
	 *
	 * @param surf Pointer to the staging surface to map.
	 * @throws std::invalid_argument If surf is null.
	 * @throws std::runtime_error If mapping fails or returns null data.
	 */
	explicit ScopedStageSurfMap(gs_stagesurf_t *surf)
		: surf_{surf},
		  mappedData_{[this]() {
			  if (!surf_) {
				  throw std::invalid_argument("Target surface cannot be null.");
			  }

			  MappedData mappedData;
			  if (!gs_stagesurface_map(surf_, &mappedData.data, &mappedData.linesize)) {
				  throw std::runtime_error("gs_stagesurface_map failed");
			  }

			  if (mappedData.data == nullptr) {
				  throw std::runtime_error("gs_stagesurface_map returned null data");
			  }

			  return mappedData;
		  }()}
	{
	}

	/**
	 * @brief Unmaps the staging surface on destruction.
	 */
	~ScopedStageSurfMap() noexcept
	{
		if (surf_) {
			gs_stagesurface_unmap(surf_);
		}
	}

	ScopedStageSurfMap(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap &operator=(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap(ScopedStageSurfMap &&) = delete;
	ScopedStageSurfMap &operator=(ScopedStageSurfMap &&) = delete;

	/**
	 * @brief Returns a pointer to the mapped pixel data.
	 * @return Pointer to the mapped data.
	 */
	const std::uint8_t *getData() const noexcept { return mappedData_.data; }

	/**
	 * @brief Returns the line size (stride) of the mapped surface.
	 * @return Line size in bytes.
	 */
	std::uint32_t getLinesize() const noexcept { return mappedData_.linesize; }

private:
	/**
	 * @brief Structure holding mapped data pointer and line size.
	 */
	struct MappedData {
		std::uint8_t *data;
		std::uint32_t linesize;
	};

	/**
	 * @brief The staging surface being managed.
	 */
	gs_stagesurf_t *const surf_;

	/**
	 * @brief The mapped data from the surface.
	 */
	const MappedData mappedData_;
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
class AsyncTextureReader final {
public:
	/**
	 * @brief Returns the number of bytes per pixel for a given color format.
	 *
	 * @param format The color format to query.
	 * @return Number of bytes per pixel for the format.
	 * @throws std::runtime_error If the format is unsupported or compressed.
	 */
	static std::uint32_t getBytesPerPixel(const gs_color_format format)
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
	/**
	 * @brief Constructs the AsyncTextureReader and allocates all necessary resources.
	 *
	 * @param width Width of the texture to read.
	 * @param height Height of the texture to read.
	 * @param format Color format of the texture.
	 * @throws std::runtime_error If staging surfaces cannot be created.
	 */
	AsyncTextureReader(const std::uint32_t width, const std::uint32_t height, const gs_color_format format)
		: width_(width),
		  height_(height),
		  bufferLinesize_(width_ * getBytesPerPixel(format)),
		  cpuBuffers_{std::vector<std::uint8_t>(static_cast<std::size_t>(height_) * bufferLinesize_),
			      std::vector<std::uint8_t>(static_cast<std::size_t>(height_) * bufferLinesize_)},
		  stagesurfs_{ObsBridgeUtils::make_unique_gs_stagesurf(width, height, format),
			      ObsBridgeUtils::make_unique_gs_stagesurf(width, height, format)}
	{
		if (!stagesurfs_[0] || !stagesurfs_[1]) {
			throw std::runtime_error("Failed to create staging surfaces");
		}
	}

	/**
	 * @brief Destroys the AsyncTextureReader and releases all allocated resources.
	 *
	 * Automatically cleans up the GPU staging surfaces (via unique_ptr)
	 * and the CPU-side pixel buffers (via std::vector).
	 */
	~AsyncTextureReader() noexcept = default;

	/**
	 * @brief Schedules a GPU texture copy. Call from the render/GPU thread.
	 *
	 * @param sourceTexture The source GPU texture to copy.
	 */
	void stage(const unique_gs_texture_t &sourceTexture) noexcept { stage(sourceTexture.get()); }

	/**
	 * @brief Schedules a GPU texture copy. Call from the render/GPU thread.
	 *
	 * @param sourceTexture The source GPU texture to copy.
	 */
	void stage(gs_texture_t *sourceTexture) noexcept
	{
		if (sourceTexture == nullptr) {
			return;
		}

		std::scoped_lock lock(gpuMutex_);
		gs_stage_texture(stagesurfs_[gpuWriteIndex_].get(), sourceTexture);
		gpuWriteIndex_ = 1 - gpuWriteIndex_;
	}

	/**
	 * @brief Synchronizes the latest texture data to the CPU buffer. Call from a CPU thread.
	 *
	 * This operation may be expensive due to GPU-to-CPU data transfer.
	 * @throws std::runtime_error If mapping the staging surface fails.
	 */
	void sync()
	{
		using namespace AsyncTextureReaderDetail;

		std::size_t gpuReadIndex;
		{
			std::scoped_lock lock(gpuMutex_);
			gpuReadIndex = 1 - gpuWriteIndex_;
		}
		gs_stagesurf_t *const stagesurf = stagesurfs_[gpuReadIndex].get();

		if (!stagesurf) {
			return;
		}

		const ScopedStageSurfMap mappedSurf(stagesurf);

		const std::size_t backBufferIndex = 1 - activeCpuBufferIndex_.load(std::memory_order_acquire);

		auto &backBuffer = cpuBuffers_[backBufferIndex];

		if (bufferLinesize_ == mappedSurf.getLinesize()) {
			std::memcpy(backBuffer.data(), mappedSurf.getData(),
				    static_cast<std::size_t>(height_) * bufferLinesize_);
		} else {
			for (std::uint32_t y = 0; y < height_; y++) {
				const std::uint8_t *srcRow = mappedSurf.getData() + (y * mappedSurf.getLinesize());
				std::uint8_t *dstRow = backBuffer.data() + (y * bufferLinesize_);
				std::size_t copyBytes =
					std::min<std::size_t>(bufferLinesize_, mappedSurf.getLinesize());
				std::memcpy(dstRow, srcRow, copyBytes);
			}
		}

		activeCpuBufferIndex_.store(backBufferIndex, std::memory_order_release);
	}

	/**
	 * @brief Returns a reference to the CPU buffer containing the latest pixel data.
	 *
	 * This operation is lock-free for immediate data access.
	 * @return Read-only buffer with the latest pixel data.
	 */
	const std::vector<std::uint8_t> &getBuffer() const noexcept
	{
		return cpuBuffers_[activeCpuBufferIndex_.load(std::memory_order_acquire)];
	}

	/**
	 * @brief Returns the width of the texture.
	 * @return Texture width in pixels.
	 */
	std::uint32_t getWidth() const noexcept { return width_; }

	/**
	 * @brief Returns the height of the texture.
	 * @return Texture height in pixels.
	 */
	std::uint32_t getHeight() const noexcept { return height_; }

	/**
	 * @brief Returns the line size (stride) of the buffer.
	 * @return Buffer line size in bytes.
	 */
	std::uint32_t getBufferLinesize() const noexcept { return bufferLinesize_; }

private:
	/**
	 * @brief Texture width in pixels.
	 */
	const std::uint32_t width_;

	/**
	 * @brief Texture height in pixels.
	 */
	const std::uint32_t height_;

	/**
	 * @brief Buffer line size (stride) in bytes.
	 */
	const std::uint32_t bufferLinesize_;

	/**
	 * @brief Double-buffered CPU pixel data.
	 */
	std::array<std::vector<std::uint8_t>, 2> cpuBuffers_;

	/**
	 * @brief Index of the currently active CPU buffer.
	 */
	std::atomic<std::size_t> activeCpuBufferIndex_ = {0};

	/**
	 * @brief Double-buffered GPU staging surfaces.
	 */
	const std::array<ObsBridgeUtils::unique_gs_stagesurf_t, 2> stagesurfs_;

	/**
	 * @brief Index of the GPU write buffer.
	 */
	std::size_t gpuWriteIndex_ = 0;

	/**
	 * @brief Mutex for synchronizing GPU buffer access.
	 */
	std::mutex gpuMutex_;
};

} // namespace ObsBridgeUtils
} // namespace KaitoTokyo
