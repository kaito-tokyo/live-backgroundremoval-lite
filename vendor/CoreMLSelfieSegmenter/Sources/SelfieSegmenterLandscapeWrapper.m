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

#import "SelfieSegmenterLandscapeWrapper.h"
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

#import "SelfieSegmenterLandscapeModel.h"

#define MODEL_INPUT_WIDTH 256
#define MODEL_INPUT_HEIGHT 144
#define EXPECTED_PIXEL_COUNT (144 * 256)

static NSString *const SelfieSegmenterLandscapeWrapperErrorDomain = @"SelfieSegmenterLandscapeWrapperErrorDomain";

@interface SelfieSegmenterLandscapeWrapper ()

@property (nonatomic, strong) VNCoreMLModel *visionModel;
@property (nonatomic, strong) VNCoreMLRequest *visionRequest;
@property (nonatomic, assign) CVPixelBufferRef cachedPixelBuffer;

@end

@implementation SelfieSegmenterLandscapeWrapper

- (nullable instancetype)init:(NSError *_Nullable *_Nullable)error
{
    self = [super init];
    if (self) {
        NSError *internalError = nil;  // Local error for propagation

        // 1. Load the Core ML model wrapper
        MLModelConfiguration *config = [[MLModelConfiguration alloc] init];
        SelfieSegmenterLandscapeModel *mlModel =
            [[SelfieSegmenterLandscapeModel alloc] initWithConfiguration:config error:&internalError];

        if (!mlModel) {
            if (error) {
                *error = internalError;
            }
            return nil;
        }

        // 2. Create the VNCoreMLModel
        _visionModel = [VNCoreMLModel modelForMLModel:mlModel.model error:&internalError];

        if (!_visionModel) {
            if (error) {
                *error = internalError;
            }
            return nil;
        }

        // 3. Create and store the VNCoreMLRequest
        _visionRequest = [[VNCoreMLRequest alloc] initWithModel:_visionModel];

        if (!_visionRequest) {
            if (error) {
                *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-100
                                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to create VNCoreMLRequest"}];
            }
            return nil;
        }
        // Allocate cached CVPixelBuffer
        _cachedPixelBuffer = NULL;
        const OSType pixelFormat = kCVPixelFormatType_32BGRA;
        NSDictionary *attrs = @{
            (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
            (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES
        };
        CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT, pixelFormat,
                                             (__bridge CFDictionaryRef)attrs, &_cachedPixelBuffer);
        if (status != kCVReturnSuccess || !_cachedPixelBuffer) {
            if (error) {
                *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-101
                                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to allocate cached CVPixelBuffer"}];
            }
            return nil;
        }
    }
    return self;
}

- (void)dealloc
{
    if (_cachedPixelBuffer) {
        CFRelease(_cachedPixelBuffer);
        _cachedPixelBuffer = NULL;
    }
}

- (nullable MLMultiArray *)performWithHandler:(VNImageRequestHandler *)handler
                                        error:(NSError *_Nullable *_Nullable)error
{
    // Step 0: Check if the request was initialized successfully
    if (!self.visionRequest) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-1
                                     userInfo:@{NSLocalizedDescriptionKey: @"VNCoreMLRequest not initialized"}];
        }
        return NULL;
    }

    // Step 1: Perform the Vision request synchronously
    BOOL success = [handler performRequests:@[self.visionRequest] error:error];

    if (!success) {
        // 'error' pointer is set by the performRequests method on failure
        return NULL;
    }

    // Step 2: Find the observation named "segmentationMask" in the results
    VNCoreMLFeatureValueObservation *featureObs = nil;

    for (VNObservation *observation in self.visionRequest.results) {
        if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
            VNCoreMLFeatureValueObservation *tempObs = (VNCoreMLFeatureValueObservation *) observation;
            if ([tempObs.featureName isEqualToString:@"segmentationMask"]) {
                featureObs = tempObs;
                break;
            }
        }
    }

    // Step 3: Check if the "segmentationMask" was found
    if (!featureObs) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-2 userInfo:@{
                NSLocalizedDescriptionKey:
                    @"'segmentationMask' feature not found or is not VNCoreMLFeatureValueObservation"
            }];
        }
        return NULL;
    }

    // Step 4: Validate that the feature type is MLMultiArray
    if (featureObs.featureValue.type != MLFeatureTypeMultiArray) {
        if (error) {
            *error = [NSError
                errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                           code:-3
                       userInfo:@{NSLocalizedDescriptionKey: @"'segmentationMask' is not MLFeatureTypeMultiArray"}];
        }
        return NULL;
    }

    MLMultiArray *multiArray = featureObs.featureValue.multiArrayValue;

    return multiArray;
}

- (nullable MLMultiArray *)performWithBGRAData:(uint8_t *_Nonnull)bgraData error:(NSError *_Nullable *_Nullable)error;
{
    // Step 1: Validate input
    if (!bgraData) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-10
                                     userInfo:@{NSLocalizedDescriptionKey: @"Input BGRA data is NULL"}];
        }
        return NULL;
    }
    if (!_cachedPixelBuffer) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-14
                                     userInfo:@{NSLocalizedDescriptionKey: @"Cached CVPixelBuffer is not allocated"}];
        }
        return NULL;
    }

    // Step 2: Copy BGRA data into cached pixel buffer
    CVPixelBufferLockBaseAddress(_cachedPixelBuffer, 0);
    void *baseAddress = CVPixelBufferGetBaseAddress(_cachedPixelBuffer);
    if (!baseAddress) {
        CVPixelBufferUnlockBaseAddress(_cachedPixelBuffer, 0);
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-12
                                     userInfo:@{NSLocalizedDescriptionKey: @"Failed to get base address of CVPixelBuffer"}];
        }
        return NULL;
    }
    memcpy(baseAddress, bgraData, MODEL_INPUT_HEIGHT * MODEL_INPUT_WIDTH * 4);
    CVPixelBufferUnlockBaseAddress(_cachedPixelBuffer, 0);
    
    // Step 3: Create VNImageRequestHandler
    VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:_cachedPixelBuffer options:@{}];
    if (!handler) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain code:-13
                                     userInfo:@{NSLocalizedDescriptionKey: @"Failed to create VNImageRequestHandler"}];
        }
        return NULL;
    }
    // Step 4: Perform segmentation
    MLMultiArray *result = [self performWithHandler:handler error:error];
    return result;
}

@end
