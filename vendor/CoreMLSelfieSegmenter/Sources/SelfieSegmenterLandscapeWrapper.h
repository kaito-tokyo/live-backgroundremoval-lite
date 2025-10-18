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
 * and returns a direct pointer to the raw mask data.
 *
 * @param handler The VNImageRequestHandler containing the image to process.
 * @param error On failure (e.g., request failed, mask not found, type mismatch, or size mismatch),
 * this pointer is set to an NSError object describing the problem.
 * @return A direct pointer to the raw `float` (Float32) data of the segmentation mask.
 * Returns `NULL` if the operation fails.
 * @warning The caller does *not* own the memory pointed to by the return value.
 * Its lifetime is tied to the underlying MLMultiArray within the VNObservation,
 * which is held by the VNCoreMLRequest's results.
 */
- (nullable float *)performWithHandler:(VNImageRequestHandler *_Nonnull)handler
                                 error:(NSError *_Nullable *_Nullable)error;

@end

NS_ASSUME_NONNULL_END
