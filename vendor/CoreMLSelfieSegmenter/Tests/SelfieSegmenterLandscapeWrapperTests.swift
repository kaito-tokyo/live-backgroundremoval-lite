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

import AppKit  // For NSImage / CGImage (macOS)
import CoreVideo
import Vision  // For VNImageRequestHandler
import XCTest

final class SelfieSegmenterLandscapeWrapperTests: XCTestCase {

  // Expected model input/output dimensions
  let modelWidth = 256
  let modelHeight = 144

  // MARK: - Test Error

  // Local error enum for test helpers
  enum TestError: Error {
    case fileNotFound(String)
    case maskLoadFailed(String)
    case maskDataExtractionFailed
  }

  // MARK: - Success Cases

  /**
   * Verifies that given a valid image, the wrapper returns a result
   * that is close to the expected ground truth mask.
   */
  func testSegmentationSuccess_ValidImage_ReturnsCorrectMask() throws {

    // 1. Prepare the wrapper and the input handler
    let segmenter = try SelfieSegmenterLandscapeWrapper()

    guard let nsImage = Bundle(for: type(of: self)).image(forResource: "selfie001"),
      let cgImage = nsImage.cgImage(forProposedRect: nil, context: nil, hints: nil)
    else {
      XCTFail("Failed to load input image 'selfie001'.")
      throw TestError.fileNotFound("selfie001")
    }
    let inputHandler = VNImageRequestHandler(cgImage: cgImage)

    // 2. Perform segmentation
    // Note: perform(with:) can throw and returns a nullable MLMultiArray.
    let resultArray = try segmenter.perform(with: inputHandler)

    // 3. Check that the returned MLMultiArray is not nil
    XCTAssertNotNil(resultArray, "perform(with:) returned nil.")

    // 4. Load the ground truth mask
    guard let nsMask = Bundle(for: type(of: self)).image(forResource: "selfie001_mask"),
      let maskCGImage = nsMask.cgImage(forProposedRect: nil, context: nil, hints: nil)
    else {
      XCTFail("Failed to load mask image 'selfie001_mask'.")
      throw TestError.maskLoadFailed("selfie001_mask")
    }

    let pixelCount = modelWidth * modelHeight

    // 5. Validate and extract ground truth mask data
    XCTAssertEqual(maskCGImage.width, modelWidth, "Ground truth mask width is incorrect.")
    XCTAssertEqual(maskCGImage.height, modelHeight, "Ground truth mask height is incorrect.")

    guard
      let truthMaskData = extractGrayscaleData(
        from: maskCGImage, width: modelWidth, height: modelHeight)
    else {
      XCTFail("Failed to extract pixel data from ground truth mask.")
      throw TestError.maskDataExtractionFailed
    }

    // 6. Convert MLMultiArray to [UInt8] array
    var modelMaskData = [UInt8](repeating: 0, count: pixelCount)
    let floatPtr = UnsafeMutableBufferPointer<Float>(
      start: resultArray.dataPointer.assumingMemoryBound(to: Float.self),
      count: pixelCount
    )
    for i in 0..<pixelCount {
      let floatValue = floatPtr[i]
      let scaledValue = floatValue * 255.0
      modelMaskData[i] = UInt8(min(max(scaledValue, 0.0), 255.0))
    }

    // 7. Compare the two masks
    // Calculate the Mean Absolute Error (MAE) per pixel.
    var totalAbsoluteDifference: Double = 0
    for i in 0..<pixelCount {
      let diff = abs(Double(modelMaskData[i]) - Double(truthMaskData[i]))
      totalAbsoluteDifference += diff
    }

    let meanAbsoluteError = totalAbsoluteDifference / Double(pixelCount)

    // Tolerance (on a 0-255 scale, allow an average error of 2.0 per pixel)
    // This value should be adjusted based on model accuracy and mask quality.
    let tolerance: Double = 2.0

    XCTAssertLessThan(
      meanAbsoluteError, tolerance,
      "Mean Absolute Error (\(meanAbsoluteError)) exceeds tolerance (\(tolerance)). Model mask is not close enough to the ground truth mask."
    )
  }

  // MARK: - Failure Cases

  /**
   * Verifies that perform(with:) correctly throws an error for
   * invalid input (e.g., empty data).
   */
  func testPerform_InvalidHandler_ThrowsError() throws {
    // 1. Successfully initialize the wrapper
    let segmenter = try SelfieSegmenterLandscapeWrapper()

    // 2. Create an invalid handler with empty Data
    let invalidHandler = VNImageRequestHandler(data: Data())

    // 3. Expect perform(with:) to throw an error
    //    (The Vision framework (VNErrorDomain) will throw the error)
    XCTAssertThrowsError(try segmenter.perform(with: invalidHandler)) { error in
      let nsError = error as NSError
      print("Caught expected error: \(nsError.domain), Code: \(nsError.code)")
      XCTAssertEqual(
        nsError.domain, VNErrorDomain, "Expected a Vision framework error (VNErrorDomain).")
    }
  }

  // MARK: - Helper Functions

  /**
   * Extracts 1-channel (grayscale) [UInt8] pixel data from a CGImage.
   * (Provided helper function)
   */
  func extractGrayscaleData(from cgImage: CGImage, width: Int, height: Int) -> [UInt8]? {
    guard cgImage.width == width, cgImage.height == height else { return nil }

    let pixelCount = width * height
    var grayscaleData = [UInt8](repeating: 0, count: pixelCount)

    // Determine if CGImage is RGBA (4ch) or Grayscale (1ch)
    let bytesPerPixel = cgImage.bitsPerPixel / 8
    let bytesPerRow = cgImage.bytesPerRow

    guard let dataProvider = cgImage.dataProvider,
      let cfdata = dataProvider.data,
      let rawPtr = CFDataGetBytePtr(cfdata)
    else {
      return nil
    }

    for y in 0..<height {
      for x in 0..<width {
        let byteIndex = (y * bytesPerRow) + (x * bytesPerPixel)

        // Use the first byte (R or Gray) as the mask value
        let maskValue = rawPtr[byteIndex]
        grayscaleData[y * width + x] = maskValue
      }
    }

    return grayscaleData
  }
}
