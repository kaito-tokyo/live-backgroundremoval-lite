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
#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Vision/Vision.h>

#import <CoreMLSelfieSegmenter/CoreMLSelfieSegmenter.h>

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
                        CFBridgingRelease((__bridge CFTypeRef) ptr);
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
                    (__bridge SelfieSegmenterLandscapeWrapper *) (CFBridgingRetain(rawWrapper)));
            }
        }  // anonymous namespace

        class CoreMLSelfieSegmenterImpl
        {
              private:
            const std::unique_ptr<SelfieSegmenterLandscapeWrapper, WrapperDeleter> wrapper;
            MaskBuffer maskBuffer;
            mutable std::mutex processMutex;

              public:
            CoreMLSelfieSegmenterImpl(std::size_t pixelCount) : wrapper(make_unique_wrapper()), maskBuffer(pixelCount)
            {}
            ~CoreMLSelfieSegmenterImpl() = default;

            void process(const std::uint8_t *bgraData, const std::size_t pixelCount)
            {
                NSError *error = nil;
                MLMultiArray *maskArray = nullptr;
                {
                    std::lock_guard<std::mutex> lock(processMutex);
                    maskArray = [wrapper.get() performWithBGRAData:bgraData error:&error];
                }
                if (maskArray) {
                    maskBuffer.write([&](std::uint8_t *dst) {
                        float *maskPtr = (float *) maskArray.dataPointer;
                        copy_float32_to_r8(dst, maskPtr, pixelCount);
                    });
                } else {
                    std::string errMsg = error ? [[error localizedDescription] UTF8String]
                                               : "Unknown error in CoreML segmentation";
                    throw std::runtime_error(errMsg);
                }
            }

            const std::uint8_t *getMask() const
            {
                return maskBuffer.read();
            }
        };

        void SelfieSegmenter::CoreMLSelfieSegmenter::process(const std::uint8_t *bgraData)
        {
            pImpl->process(bgraData, getPixelCount());
        }

        const std::uint8_t *SelfieSegmenter::CoreMLSelfieSegmenter::getMask() const
        {
            return pImpl->getMask();
        }

    }  // namespace SelfieSegmenter
}  // namespace KaitoTokyo
