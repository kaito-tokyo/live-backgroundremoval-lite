#include "gtest/gtest.h"
#include <vector>
#include <numeric>

// テスト対象のヘッダーをインクルード
#include "SelfieSegmenter/UyvyFrameExtractor.hpp"

// kaito_tokyo::... 名前空間をエイリアス
namespace kse = kaito_tokyo::obs_backgroundremoval_lite::selfie_segmenter;

//==============================================================================
// 1. calculate_downsampled_dims 関数のテスト
//==============================================================================

TEST(CalculateDownsampledDimsTest, HandlesZeroInput) {
    auto dims = kse::calculate_downsampled_dims(1920, 0);
    EXPECT_EQ(dims.first, 0);
    EXPECT_EQ(dims.second, 0);
}

TEST(CalculateDownsampledDimsTest, LandscapeHD) {
    // 1920x1080 (16:9) -> 256x144
    auto dims = kse::calculate_downsampled_dims(1920, 1080, 256);
    EXPECT_EQ(dims.first, 256);
    EXPECT_EQ(dims.second, 144);
}

TEST(CalculateDownsampledDimsTest, Portrait) {
    // 1080x1920 (9:16) -> 144x256
    auto dims = kse::calculate_downsampled_dims(1080, 1920, 256);
    EXPECT_EQ(dims.first, 144);
    EXPECT_EQ(dims.second, 256);
}

TEST(CalculateDownsampledDimsTest, Square) {
    // 1024x1024 -> 256x256
    auto dims = kse::calculate_downsampled_dims(1024, 1024, 256);
    EXPECT_EQ(dims.first, 256);
    EXPECT_EQ(dims.second, 256);
}

TEST(CalculateDownsampledDimsTest, SmallerThanMax) {
    // 200x100 -> 200x100 (変更なし)
    auto dims = kse::calculate_downsampled_dims(200, 100, 256);
    EXPECT_EQ(dims.first, 200);
    EXPECT_EQ(dims.second, 100);
}


//==============================================================================
// 2. BaseUyvyFrameExtractor クラスのテスト
//==============================================================================

class UyvyFrameExtractorTest : public ::testing::Test {
protected:
    static constexpr size_t CANVAS_DIM = 256;
    static constexpr size_t CANVAS_SIZE = CANVAS_DIM * CANVAS_DIM;

    // 出力先のRGBバッファ
    std::vector<float> r, g, b;
    // 入力UYVYデータ
    std::vector<uint8_t> uyvy_data;

    // ▼▼▼【修正点 1】▼▼▼
    // operator() に渡すためのデータプレーン配列 (UYVYはシングルプレーンなので要素数は1)
    const void* data_planes_array[1]; 
    // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲

    // ▼▼▼【修正点 2】コンストラクタを削除 ▼▼▼
    // UyvyFrameExtractorTest() : src_planes(&src_data_ptr) {}

    void SetUp() override {
        // 各テストの前にバッファをリサイズ
        r.resize(CANVAS_SIZE);
        g.resize(CANVAS_SIZE);
        b.resize(CANVAS_SIZE);
        data_planes_array[0] = nullptr; // 安全のため初期化
    }

    // 特定の色でUYVY画像を生成するヘルパー関数
    void create_solid_color_uyvy_image(size_t width, size_t height, uint8_t y, uint8_t u, uint8_t v) {
        // UYVYは2ピクセルで4バイト。幅は偶数である必要がある
        ASSERT_EQ(width % 2, 0) << "Image width must be even for UYVY format.";
        uyvy_data.resize(width * height * 2);
        for (size_t i = 0; i < width * height / 2; ++i) {
            uyvy_data[i * 4 + 0] = u;
            uyvy_data[i * 4 + 1] = y;
            uyvy_data[i * 4 + 2] = v;
            uyvy_data[i * 4 + 3] = y;
        }
        // ▼▼▼【修正点 3】▼▼▼
        data_planes_array[0] = uyvy_data.data();
        // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲
    }
};

// 0x0の入力で、キャンバス全体が黒(0.0f)になるかをテスト
TEST_F(UyvyFrameExtractorTest, HandlesEmptyInput) {
    kse::UyvyLimitedRec709FrameExtractor extractor;
    // ▼▼▼【修正点 4】▼▼▼
    extractor(r.data(), g.data(), b.data(), data_planes_array, 0, 0, 0);
    // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲
    
    // 全てのピクセルが0.0fであることを確認
    for (size_t i = 0; i < CANVAS_SIZE; ++i) {
        EXPECT_FLOAT_EQ(r[i], 0.0f);
        EXPECT_FLOAT_EQ(g[i], 0.0f);
        EXPECT_FLOAT_EQ(b[i], 0.0f);
    }
}

// グレースケール画像の変換とレターボックスをテスト
TEST_F(UyvyFrameExtractorTest, GrayConversionAndLetterbox) {
    const size_t SRC_W = 1280, SRC_H = 720; // 16:9
    create_solid_color_uyvy_image(SRC_W, SRC_H, 128, 128, 128);

    kse::UyvyLimitedRec709FrameExtractor extractor;
    // ▼▼▼【修正点 4】▼▼▼
    extractor(r.data(), g.data(), b.data(), data_planes_array, SRC_W, SRC_H, SRC_W * 2);
    // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲
    
    // ... (期待値の計算とアサーションは変更なし)
    auto [img_w, img_h] = kse::calculate_downsampled_dims(SRC_W, SRC_H, CANVAS_DIM);
    size_t offset_x = (CANVAS_DIM - img_w) / 2;
    size_t offset_y = (CANVAS_DIM - img_h) / 2;
    const float expected_gray = 130.41095f / 255.0f;

    size_t center_x = offset_x + img_w / 2;
    size_t center_y = offset_y + img_h / 2;
    size_t center_idx = center_y * CANVAS_DIM + center_x;
    ASSERT_NEAR(r[center_idx], expected_gray, 1e-4);
    ASSERT_NEAR(g[center_idx], expected_gray, 1e-4);
    ASSERT_NEAR(b[center_idx], expected_gray, 1e-4);
    // ... (以下略)
}

// Full RangeとLimited Rangeでの出力の違いをテスト
TEST_F(UyvyFrameExtractorTest, FullVsLimitedRange) {
    const size_t SRC_W = 256, SRC_H = 256;
    create_solid_color_uyvy_image(SRC_W, SRC_H, 16, 128, 128);

    // Limited Rangeでの変換
    kse::UyvyLimitedRec709FrameExtractor limited_extractor;
    // ▼▼▼【修正点 4】▼▼▼
    limited_extractor(r.data(), g.data(), b.data(), data_planes_array, SRC_W, SRC_H, SRC_W * 2);
    // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲
    float limited_val = r[0];

    // Full Rangeでの変換
    kse::UyvyFullRec709FrameExtractor full_extractor;
    // ▼▼▼【修正点 4】▼▼▼
    full_extractor(r.data(), g.data(), b.data(), data_planes_array, SRC_W, SRC_H, SRC_W * 2);
    // ▲▲▲▲▲▲▲▲▲▲▲▲▲▲
    float full_val = r[0];

    EXPECT_FLOAT_EQ(limited_val, 0.0f);
    EXPECT_FLOAT_EQ(full_val, 16.0f / 255.0f);
    EXPECT_NE(limited_val, full_val);
}
