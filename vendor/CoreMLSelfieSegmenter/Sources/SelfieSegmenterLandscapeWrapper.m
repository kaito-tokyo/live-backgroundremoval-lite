// SelfieSegmenterLandscapeWrapper.m

#import "SelfieSegmenterLandscapeWrapper.h"
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

#import "SelfieSegmenterLandscapeModel.h"

// 期待されるテンソルのサイズ (1 * 1 * 144 * 256)
#define EXPECTED_PIXEL_COUNT (144 * 256)

@interface SelfieSegmenterLandscapeWrapper ()

@property (nonatomic, strong) VNCoreMLModel *visionModel;
@property (nonatomic, strong) VNCoreMLRequest *visionRequest;

@end


@implementation SelfieSegmenterLandscapeWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        NSError *error = nil;
        
        // 1. CoreMLモデルラッパーを読み込む
        MLModelConfiguration *config = [[MLModelConfiguration alloc] init];
        SelfieSegmenterLandscapeModel *tempModel = [[SelfieSegmenterLandscapeModel alloc] initWithConfiguration:config error:&error];

        if (!tempModel) {
            NSLog(@"[MySegmenterBridge] Failed to load CoreML model wrapper: %@", error);
            return nil;
        }
        
        // 2. VNCoreMLModel を作成
        _visionModel = [VNCoreMLModel modelForMLModel:tempModel.model error:&error];
        
        if (!_visionModel) {
            NSLog(@"[MySegmenterBridge] Failed to create VNCoreMLModel: %@", error);
            return nil;
        }
        
        // 3. VNCoreMLRequest を一度だけ作成し、プロパティとして保持
        _visionRequest = [[VNCoreMLRequest alloc] initWithModel:_visionModel];
        
        if (!_visionRequest) {
            NSLog(@"[MySegmenterBridge] Failed to create VNCoreMLRequest");
            return nil;
        }
    }
    return self;
}

// [修正] 戻り値を VNObservation* から float* に変更
- (nullable float *)performSegmentationWithHandler:(VNImageRequestHandler *)handler
                                              error:(NSError *_Nullable *_Nullable)error {
                                                      
    if (!self.visionRequest) { // initで失敗していた場合
        if (error) {
            *error = [NSError errorWithDomain:@"MySegmenterBridgeError"
                                         code:-1
                                     userInfo:@{NSLocalizedDescriptionKey: @"VNCoreMLRequest not initialized"}];
        }
        return NULL;
    }

    // 1. リクエストを同期的に実行
    BOOL success = [handler performRequests:@[self.visionRequest] error:error];

    if (!success) {
        // 'error' ポインタは performRequests メソッドが設定してくれます
        return NULL;
    }
    
    // 2. results から "segmentationMask" を探す
    VNCoreMLFeatureValueObservation *featureObs = nil;
    
    for (VNObservation *observation in self.visionRequest.results) {
        if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
            VNCoreMLFeatureValueObservation *tempObs = (VNCoreMLFeatureValueObservation *)observation;
            if ([tempObs.featureName isEqualToString:@"segmentationMask"]) {
                featureObs = tempObs;
                break;
            }
        }
    }
    
    // 3. 目的の Observation が見つからなかった場合
    if (!featureObs) {
        if (error) {
            *error = [NSError errorWithDomain:@"MySegmenterBridgeError"
                                         code:-2
                                     userInfo:@{NSLocalizedDescriptionKey: @"'segmentationMask' feature not found or is not VNCoreMLFeatureValueObservation"}];
        }
        return NULL;
    }

    // 4. [修正] MLMultiArray を取り出して検証
    if (featureObs.featureValue.type != MLFeatureTypeMultiArray) {
        if (error) {
            *error = [NSError errorWithDomain:@"MySegmenterBridgeError" code:-3
                                     userInfo:@{NSLocalizedDescriptionKey: @"'segmentationMask' is not MLFeatureTypeMultiArray"}];
        }
        return NULL;
    }
    
    MLMultiArray *multiArray = featureObs.featureValue.multiArrayValue;

    // 5. [修正] データ型とサイズを検証
    if (multiArray.dataType != MLMultiArrayDataTypeFloat32) {
        if (error) {
            *error = [NSError errorWithDomain:@"MySegmenterBridgeError" code:-4
                                     userInfo:@{NSLocalizedDescriptionKey: @"MultiArray is not Float32"}];
        }
        return NULL;
    }

    if (multiArray.count != EXPECTED_PIXEL_COUNT) {
        if (error) {
            NSString *desc = [NSString stringWithFormat:@"Unexpected MultiArray count. Expected %d, got %lld",
                              EXPECTED_PIXEL_COUNT, (long long)multiArray.count];
            *error = [NSError errorWithDomain:@"MySegmenterBridgeError" code:-5
                                     userInfo:@{NSLocalizedDescriptionKey: desc}];
        }
        return NULL;
    }
    
    // 6. [修正] データポインタ (float*) を返す
    return (float *)multiArray.dataPointer;
}

@end
