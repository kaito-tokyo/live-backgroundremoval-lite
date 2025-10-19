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

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "ShapeConverter.hpp"

namespace KaitoTokyo {
    namespace SelfieSegmenter {

        // --- Deleters for RAII ---

        // C-style Core Graphics/Core Foundation objects (ARC does not manage this)
        // This deleter remains unchanged and is correct.
        std::function<void(void *)> cgDeleter = [](void *obj) {
            if (obj)
                CFRelease(obj);
        };

        // --- Pimpl Implementation ---

        class CoreMLSelfieSegmenterImpl
        {
              private:
            static void wrapperDeleter(SelfieSegmenterLandscapeWrapper *ptr)
            {
                if (ptr)
                    CFBridgingRelease(ptr);
            }

              public:
            std::mutex mutex;
            std::unique_ptr<SelfieSegmenterLandscapeWrapper, decltype(&wrapperDeleter)> wrapper;

            CoreMLSelfieSegmenterImpl() : wrapper(nullptr, &wrapperDeleter)
            {
                NSError *error = nil;
                // ARC manages rawWrapper in this local scope
                SelfieSegmenterLandscapeWrapper *rawWrapper = [[SelfieSegmenterLandscapeWrapper alloc] init:&error];
                if (!rawWrapper) {
                    throw std::runtime_error([[error localizedDescription] UTF8String]);
                }
                // Transfer ownership to the C++ unique_ptr via CFBridgingRetain
                wrapper.reset((SelfieSegmenterLandscapeWrapper *) CFBridgingRetain(rawWrapper));
            }

            // Destructor is default, unique_ptr calls wrapperDeleter automatically
            ~CoreMLSelfieSegmenterImpl() = default;
        };

        // --- Public Interface Implementation ---

        CoreMLSelfieSegmenter::CoreMLSelfieSegmenter() :
            maskBuffer(getPixelCount()), pImpl(std::make_unique<CoreMLSelfieSegmenterImpl>())
        {}

        CoreMLSelfieSegmenter::~CoreMLSelfieSegmenter() = default;

        void CoreMLSelfieSegmenter::process(const std::uint8_t *bgraData)
        {
            std::lock_guard<std::mutex> lock(pImpl->mutex);

            // --- Core Graphics (C-API) Resource Management ---
            // This part is C, not Obj-C, so ARC does not apply.
            // Using unique_ptr is still the correct RAII pattern.
            const size_t width = getWidth();
            const size_t height = getHeight();
            const size_t pixelCount = getPixelCount();
            const size_t bytesPerRow = width * 4;

            std::unique_ptr<std::remove_pointer_t<CGColorSpaceRef>, decltype(cgDeleter)> colorSpace(
                CGColorSpaceCreateDeviceRGB(), cgDeleter);

            std::unique_ptr<std::remove_pointer_t<CGContextRef>, decltype(cgDeleter)> context(
                CGBitmapContextCreate((void *) bgraData, width, height, 8, bytesPerRow, colorSpace.get(),
                                      kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst),
                cgDeleter);

            std::unique_ptr<std::remove_pointer_t<CGImageRef>, decltype(cgDeleter)> imageRef(
                CGBitmapContextCreateImage(context.get()), cgDeleter);

            if (!imageRef) {
                throw std::runtime_error("Failed to create CGImage from BGRA data");
            }

            // --- Objective-C (ARC) Resource Management ---
            // No unique_ptr needed here. ARC will manage 'handler' automatically.
            VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCGImage:imageRef.get()
                                                                                    options:@ {}];

            // imageRef is released here (by its unique_ptr)

            NSError *error = nil;
            // Use .get() for the wrapper, but the plain 'handler' pointer
            float *maskPtr = [pImpl->wrapper.get() performWithHandler:handler error:&error];

            // [handler release] is no longer needed, ARC will handle it
            // even if an exception is thrown.

            if (!maskPtr) {
                std::string errMsg = error ? [[error localizedDescription] UTF8String]
                                           : "Unknown error in CoreML segmentation";
                throw std::runtime_error(errMsg);
            }

            // Convert float mask (0.0-1.0) to uint8_t mask (0-255)
            maskBuffer.write([&](std::uint8_t *dst) {
                KaitoTokyo::SelfieSegmenter::copy_float32_to_r8(dst, maskPtr, pixelCount);
            });
        }

    }  // namespace SelfieSegmenter
}  // namespace KaitoTokyo
