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

#define EXPECTED_PIXEL_COUNT (144 * 256)

static NSString *const SelfieSegmenterLandscapeWrapperErrorDomain = @"SelfieSegmenterLandscapeWrapperErrorDomain";

@interface SelfieSegmenterLandscapeWrapper ()

@property (nonatomic, strong) VNCoreMLModel *visionModel;
@property (nonatomic, strong) VNCoreMLRequest *visionRequest;

@end

@implementation SelfieSegmenterLandscapeWrapper

- (nullable instancetype)init:(NSError *_Nullable *_Nullable)error
{
    self = [super init];
    if (self) {
        NSError *internalError = nil; // Local error for propagation

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
                *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                                             code:-100
                                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to create VNCoreMLRequest"}];
            }
            return nil;
        }
    }
    return self;
}

- (nullable float *)performWithHandler:(VNImageRequestHandler *)handler error:(NSError *_Nullable *_Nullable)error
{
    // Step 0: Check if the request was initialized successfully
    if (!self.visionRequest) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                                         code:-1
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
            *error = [NSError
                errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                           code:-2
                       userInfo:@{
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

    // Step 5: Validate the data type (Float32) and element count (EXPECTED_PIXEL_COUNT)
    if (multiArray.dataType != MLMultiArrayDataTypeFloat32) {
        if (error) {
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                                         code:-4
                                     userInfo:@{NSLocalizedDescriptionKey: @"MultiArray is not Float32"}];
        }
        return NULL;
    }

    if (multiArray.count != EXPECTED_PIXEL_COUNT) {
        if (error) {
            NSString *desc = [NSString stringWithFormat:@"Unexpected MultiArray count. Expected %d, got %lld",
                                                        EXPECTED_PIXEL_COUNT, (long long) multiArray.count];
            *error = [NSError errorWithDomain:SelfieSegmenterLandscapeWrapperErrorDomain
                                         code:-5
                                     userInfo:@{NSLocalizedDescriptionKey: desc}];
        }
        return NULL;
    }

    // Step 6: Return the raw float data pointer
    return (float *) multiArray.dataPointer;
}

@end
