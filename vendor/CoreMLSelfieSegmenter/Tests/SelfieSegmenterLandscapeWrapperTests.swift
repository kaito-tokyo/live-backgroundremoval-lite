import XCTest
import CoreVideo
import Vision // VNImageRequestHandler を使うため
import AppKit // NSImage / CGImage のためにインポート (macOSの場合)
// import UIKit // (iOSの場合)

// Bridging Header 経由で SelfieSegmenterLandscapeWrapper が
// import されていることを前提とします。

final class SelfieSegmenterLandscapeWrapperTests: XCTestCase {

    // モデルの期待する入出力サイズ
    let modelWidth = 256
    let modelHeight = 144

    // MARK: - Test Cases

    func testSegmentationSuccess() throws {
        // 1. モデルのセットアップ
        // (モデルファイルがテストバンドルに追加されている必要がある)
        let segmenter = SelfieSegmenterLandscapeWrapper()
        
        // 2. 入力画像のロード
        let bundle = Bundle(for: type(of: self))
        guard let nsImage = bundle.image(forResource: "selfie001"),
              let cgImage = nsImage.cgImage(forProposedRect: nil, context: nil, hints: nil)
        else {
            XCTFail("Failed to load input image 'selfie001'.")
            return
        }
        
        // 3. セグメンテーション実行
        let inputHandler = VNImageRequestHandler(cgImage: cgImage)
        var error: NSError?
        
        // segmenter.performSegmentation は (float *) を返す
        let resultFloatPtr = try! segmenter.performSegmentation(with: inputHandler)
        // 4. 正解マスクのロード
        guard let nsMask = bundle.image(forResource: "selfie001_mask"),
              let maskCGImage = nsMask.cgImage(forProposedRect: nil, context: nil, hints: nil)
        else {
            XCTFail("Failed to load mask image 'selfie001_mask'.")
            return
        }
        
        let pixelCount = modelWidth * modelHeight
        
        // 5. 正解マスクのデータ検証と抽出
        XCTAssertEqual(maskCGImage.width, modelWidth, "Ground truth mask width is incorrect.")
        XCTAssertEqual(maskCGImage.height, modelHeight, "Ground truth mask height is incorrect.")
        
        guard let truthMaskData = extractGrayscaleData(from: maskCGImage, width: modelWidth, height: modelHeight) else {
            XCTFail("Failed to extract pixel data from ground truth mask.")
            return
        }

        // 6. モデル出力 (float*) を [UInt8] 配列に変換
        var modelMaskData = [UInt8](repeating: 0, count: pixelCount)
        for i in 0..<pixelCount {
            let floatValue = resultFloatPtr[i]
            
            // --- ⚠️ モデルの出力範囲に関する仮定 ---
            // モデルが 0.0-1.0 の範囲のfloatを返すと仮定し、255を乗算する。
            // もしモデルが 0.0-255.0 の範囲のfloatを返す場合、 * 255.0 は不要。
            let scaledValue = floatValue * 255.0
            
            // 値を 0-255 の UInt8 にクリッピング
            modelMaskData[i] = UInt8(min(max(scaledValue, 0.0), 255.0))
        }
        
        // 7. 2つのマスクを比較
        // ピクセルごとの完全一致は浮動小数点の誤差で失敗する可能性があるため、
        // ピクセルごとの「平均絶対誤差 (Mean Absolute Error)」を計算します。
        
        var totalAbsoluteDifference: Double = 0
        for i in 0..<pixelCount {
            let diff = abs(Double(modelMaskData[i]) - Double(truthMaskData[i]))
            totalAbsoluteDifference += diff
        }
        
        let meanAbsoluteError = totalAbsoluteDifference / Double(pixelCount)
        
        // 許容誤差 (0-255 のスケールで、ピクセルあたり平均 2.0 の誤差まで許容)
        // この値は、モデルの精度と正解マスクの品質に応じて調整してください。
        let tolerance: Double = 2.0
        
        XCTAssertLessThan(meanAbsoluteError, tolerance,
                          "Mean Absolute Error (\(meanAbsoluteError)) exceeds tolerance (\(tolerance)). Model mask is not close enough to the ground truth mask.")
    }
    
    // MARK: - Helper Functions
    
    /**
     * CGImage から 1ch (グレースケール) の [UInt8] ピクセルデータを抽出します。
     *
     * @param cgImage 入力画像
     * @param width 期待される幅
     * @param height 期待される高さ
     * @return 1ピクセル1バイトの UInt8 配列
     */
    func extractGrayscaleData(from cgImage: CGImage, width: Int, height: Int) -> [UInt8]? {
        guard cgImage.width == width, cgImage.height == height else { return nil }
        
        let pixelCount = width * height
        var grayscaleData = [UInt8](repeating: 0, count: pixelCount)
        
        // CGImage が RGBA (4ch) か Grayscale (1ch) かを判別
        let bytesPerPixel = cgImage.bitsPerPixel / 8
        let bytesPerRow = cgImage.bytesPerRow
        
        guard let dataProvider = cgImage.dataProvider,
              let cfdata = dataProvider.data,
              let rawPtr = CFDataGetBytePtr(cfdata) else {
            return nil
        }
        
        for y in 0..<height {
            for x in 0..<width {
                let byteIndex = (y * bytesPerRow) + (x * bytesPerPixel)
                
                // 最初のバイト (R または Gray) をマスク値として使用
                let maskValue = rawPtr[byteIndex]
                grayscaleData[y * width + x] = maskValue
            }
        }
        
        return grayscaleData
    }
}
