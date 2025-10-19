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

#import "CoreMLSelfieSegmenter.hpp"

#import <Foundation/Foundation.h>
#import <CoreMLSelfieSegmenter/SelfieSegmenterLandscapeWrapper.h>
#import <Vision/Vision.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreFoundation/CoreFoundation.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ShapeConverter.hpp"

namespace KaitoTokyo {
    namespace SelfieSegmenter {
        namespace {
            struct WrapperDeleter {
                void operator()(SelfieSegmenterLandscapeWrapper *ptr) const
                {
                    if (ptr) {
                        CFRelease(ptr);
                    }
                }
            };

            inline std::unique_ptr<SelfieSegmenterLandscapeWrapper, WrapperDeleter> make_unique_wrapper()
            {
                NSError *error = nil;

                // ARC manages rawWrapper in this local scope
                SelfieSegmenterLandscapeWrapper *rawWrapper = [[SelfieSegmenterLandscapeWrapper alloc] init:&error];
                if (!rawWrapper) {
                    throw std::runtime_error([[error localizedDescription] UTF8String]);
                }

                // Transfer ownership to the C++ unique_ptr via CFBridgingRetain
                return std::unique_ptr<SelfieSegmenterLandscapeWrapper, WrapperDeleter>(
                    reinterpret_cast<SelfieSegmenterLandscapeWrapper *>(CFBridgingRetain(rawWrapper)));
            }
        }  // anonymous namespace

        class CoreMLSelfieSegmenterImpl
        {
              private:
            std::unique_ptr<SelfieSegmenterLandscapeWrapper, WrapperDeleter> wrapper;

              public:
            CoreMLSelfieSegmenterImpl() : wrapper(std::move(make_unique_wrapper())) {}

            void process(const std::uint8_t *bgraData)
            {
                const size_t pixelCount = getPixelCount();

                NSError *error = nil;
                MLMultiArray *maskArray = [wrapper.get() performWithBGRAData:bgraData error:&error];
                if (!maskArray) {
                    std::string errMsg = error ? [[error localizedDescription] UTF8String]
                                               : "Unknown error in CoreML segmentation";
                    throw std::runtime_error(errMsg);
                }

                // Convert float mask (0.0-1.0) to uint8_t mask (0-255)
                maskBuffer.write([&](std::uint8_t *dst) {
                    float *maskPtr = (float *) maskArray.dataPointer;
                    KaitoTokyo::SelfieSegmenter::copy_float32_to_r8(dst, maskPtr, pixelCount);
                });
            }
        };

        // --- Public Interface Implementation ---

        CoreMLSelfieSegmenter::CoreMLSelfieSegmenter() :
            maskBuffer(getPixelCount()), pImpl(std::make_unique<CoreMLSelfieSegmenterImpl>())
        {}

        CoreMLSelfieSegmenter::~CoreMLSelfieSegmenter() = default;

        void CoreMLSelfieSegmenter::process(const std::uint8_t *bgraData)
        {
            std::lock_guard<std::mutex> lock(pImpl->mutex);

            const size_t pixelCount = getPixelCount();

            NSError *error = nil;
            MLMultiArray *maskArray = [pImpl->wrapper.get() performWithBGRAData:(uint8_t *) bgraData error:&error];
            if (!maskArray) {
                std::string errMsg = error ? [[error localizedDescription] UTF8String]
                                           : "Unknown error in CoreML segmentation";
                throw std::runtime_error(errMsg);
            }

            // Convert float mask (0.0-1.0) to uint8_t mask (0-255)
            maskBuffer.write([&](std::uint8_t *dst) {
                float *maskPtr = (float *) maskArray.dataPointer;
                KaitoTokyo::SelfieSegmenter::copy_float32_to_r8(dst, maskPtr, pixelCount);
            });
        }

    }  // namespace SelfieSegmenter
}  // namespace KaitoTokyo
