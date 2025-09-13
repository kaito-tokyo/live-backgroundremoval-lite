/*
OBS Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <gtest/gtest.h>
#import <opencv2/opencv.hpp>

#import <Foundation/Foundation.h>
#import <Vision/Vision.h>
#import <CoreGraphics/CoreGraphics.h>

// Objective-C++なのでC++のusing宣言も可能
using namespace cv;

/**
 * @brief OpenCVのMatオブジェクト（BGR形式）をCore GraphicsのCGImageRefに変換します。
 * @param image BGR形式の入力cv::Mat
 * @return 変換されたCGImageRefオブジェクト。呼び出し元でCFRelease()で解放する必要があります。
 */
CGImageRef MatToCGImage(const Mat &image)
{
    if (image.empty() || !(image.type() == CV_8UC3)) {
        return NULL;
    }

    // OpenCVのMatはBGR形式なので、Visionフレームワークで扱うためにRGB形式に変換
    Mat rgbImage;
    cvtColor(image, rgbImage, COLOR_BGR2RGB);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    if (colorSpace == NULL) {
        fprintf(stderr, "Failed to create color space.\n");
        return NULL;
    }

    // MatのデータからCGDataProviderRefを作成
    NSData *data = [NSData dataWithBytes:rgbImage.data length:rgbImage.total() * rgbImage.elemSize()];
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((CFDataRef) data);
    if (provider == NULL) {
        fprintf(stderr, "Failed to create data provider.\n");
        CGColorSpaceRelease(colorSpace);
        return NULL;
    }

    CGImageRef cgImage = CGImageCreate(rgbImage.cols,  // 幅
                                       rgbImage.rows,  // 高さ
                                       8,              // 1コンポーネントあたりのビット数
                                       24,             // 1ピクセルあたりのビット数 (8 bits * 3 channels)
                                       rgbImage.step,  // 1行あたりのバイト数
                                       colorSpace,     // カラースペース
                                       kCGBitmapByteOrderDefault | kCGImageAlphaNone,  // ビットマップ情報
                                       provider,                                       // データプロバイダ
                                       NULL,                                           // デコード配列
                                       false,                                          // shouldInterpolate
                                       kCGRenderingIntentDefault                       // レンダリングインテント
    );

    // 作成したオブジェクトを解放
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);

    return cgImage;
}

/**
 * @brief Visionフレームワークから返されたCVPixelBufferRef（セグメンテーションマスク）をOpenCVのMatに変換します。
 * @param pixelBuffer 入力CVPixelBufferRef (kCVPixelFormatType_OneComponent8)
 * @return 変換されたグレースケールのcv::Matオブジェクト
 */
Mat PixelBufferToMat(CVPixelBufferRef pixelBuffer)
{
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

    void *baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

    // Visionの人物セグメンテーションマスクは通常、CV_8UC1形式に合致します
    Mat mask(static_cast<int>(height), static_cast<int>(width), CV_8UC1, baseAddress, bytesPerRow);

    // pixelBufferのロックが解除されるとbaseAddressは無効になるため、データをコピー（クローン）する
    Mat resultMat = mask.clone();

    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

    return resultMat;
}

TEST(StubTest, Stub)
{
    @autoreleasepool {
        // 1. OpenCVで画像を読み込む
        std::string imagePath = "image.png";
        Mat originalImage = imread(imagePath);
        if (originalImage.empty()) {
            printf("エラー: 画像ファイルを読み込めませんでした: %s\n", imagePath.c_str());
            return;
        }

        // 2. OpenCVのMatをVisionが扱えるCGImageRefに変換
        CGImageRef cgImage = MatToCGImage(originalImage);
        if (cgImage == NULL) {
            printf("エラー: MatからCGImageへの変換に失敗しました。\n");
            return;
        }

        // 3. Visionフレームワークで人物セグメンテーションリクエストを作成
        // 高品質なマスクを生成するために .accurate を指定
        VNGeneratePersonSegmentationRequest *request = [[VNGeneratePersonSegmentationRequest alloc] init];
        request.qualityLevel = VNGeneratePersonSegmentationRequestQualityLevelFast;

        // 4. 画像リクエストハンドラを作成し、リクエストを実行
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCGImage:cgImage options:@ {}];

        NSError *error = nil;
        if (![handler performRequests:@[request] error:&error]) {
            NSLog(@"リクエストの実行に失敗しました: %@", error);
            CFRelease(cgImage);
            return;
        }

        // 5. 結果を処理する
        VNPixelBufferObservation *result = request.results.firstObject;
        if (!result) {
            printf("人物が見つかりませんでした。\n");
            CFRelease(cgImage);
            return;
        }

        // 6. 結果のマスク(CVPixelBufferRef)をOpenCVのMatに変換
        Mat mask = PixelBufferToMat(result.pixelBuffer);

        // Visionのマスクは値が0か1なので、表示や後処理のために255倍して見やすくする
        Mat binaryMask;
        threshold(mask, binaryMask, 0, 255, THRESH_BINARY);

        // 7. マスクのサイズを元の画像に合わせてリサイズし、人物を切り抜く
        Mat resizedMask;
        // binaryMaskをoriginalImageと同じサイズにリサイズする
        // INTER_NEARESTは、白黒のマスク画像をリサイズするのに最も適した補間方法
        resize(binaryMask, resizedMask, originalImage.size(), 0, 0, INTER_NEAREST);

        Mat segmentedImage = Mat::zeros(originalImage.size(), originalImage.type());
        // リサイズしたマスクを使って切り抜く
        originalImage.copyTo(segmentedImage, resizedMask);

        // 8. 結果をファイルに保存
        std::string outputMaskPath = "result_mask.png";
        std::string outputSegmentedPath = "result_segmented.png";

        bool isMaskSaved = imwrite(outputMaskPath, resizedMask);  // 保存するマスクもリサイズ後のものに
        if (!isMaskSaved) {
            printf("エラー: マスク画像の保存に失敗しました: %s\n", outputMaskPath.c_str());
        } else {
            printf("マスク画像を保存しました: %s\n", outputMaskPath.c_str());
        }

        bool isSegmentedSaved = imwrite(outputSegmentedPath, segmentedImage);
        if (!isSegmentedSaved) {
            printf("エラー: 切り抜き画像の保存に失敗しました: %s\n", outputSegmentedPath.c_str());
        } else {
            printf("切り抜き画像を保存しました: %s\n", outputSegmentedPath.c_str());
        }

        ASSERT_TRUE(isMaskSaved);
        ASSERT_TRUE(isSegmentedSaved);

        // CGImageRefを解放
        CFRelease(cgImage);
    }
}
