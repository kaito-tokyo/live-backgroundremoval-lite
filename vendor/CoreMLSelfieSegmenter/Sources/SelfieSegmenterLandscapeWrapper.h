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

#pragma once

#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

NS_ASSUME_NONNULL_BEGIN

@interface SelfieSegmenterLandscapeWrapper : NSObject

/**
 * Initializes the wrapper by loading the Core ML model and setting up the Vision request.
 *
 * This initializer loads the underlying `SelfieSegmenterLandscapeModel`, wraps it in a
 * `VNCoreMLModel`, and prepares a `VNCoreMLRequest` for future predictions.
 * This method will fail (return nil and populate the error) if any of these steps fail.
 * This initializer is recognized as `init() throws` in Swift.
 *
 * @param error On failure, this pointer is set to an NSError object describing the problem.
 * @return An initialized wrapper instance, or `nil` if an error occurred.
 */
- (nullable instancetype)init:(NSError *_Nullable *_Nullable)error NS_SWIFT_NAME(init());

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/**
 * Performs synchronous segmentation on the given image request handler.
 *
 * This method executes the prepared Vision request using the provided handler.
 * It retrieves the "segmentationMask" output, validates its type (Float32) and dimensions,
 * and returns the MLMultiArray containing the mask data.
 *
 * @param handler The VNImageRequestHandler containing the image to process.
 * @param error On failure (e.g., request failed, mask not found, type mismatch, or size mismatch),
 * this pointer is set to an NSError object describing the problem.
 * @return The MLMultiArray (Float32) of the segmentation mask, or NULL if the operation fails.
 * @warning The caller does *not* own the returned MLMultiArray. Its lifetime is tied to the underlying
 * VNObservation held by the VNCoreMLRequest's results.
 */
- (nullable MLMultiArray *)performWithHandler:(VNImageRequestHandler *_Nonnull)handler
                                        error:(NSError *_Nullable *_Nullable)error;

- (nullable MLMultiArray *)performWithBGRAData:(uint8_t *_Nonnull)bgraData
                                         error:(NSError *_Nullable *_Nullable)error;

@end

NS_ASSUME_NONNULL_END
