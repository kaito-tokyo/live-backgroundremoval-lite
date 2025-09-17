#include <gtest/gtest.h>
#include "VideoFrameExtractor/UyvyFrameExtractor.hpp" // ヘッダーファイル名を実際のパスに合わせてください
#include <vector>
#include <array>
#include <cmath> // For std::abs
#include <type_traits> // For std::is_same_v

// テストデータやヘルパー関数を名前空間でカプセル化
namespace TestData {

// 1. RGBAの元画像データを定義する
namespace Colors {
    struct ColorRGBA { std::uint8_t r, g, b, a; };
    constexpr ColorRGBA RED   = {255, 0,   0,   255};
    constexpr ColorRGBA GREEN = {0,   255, 0,   255};
    constexpr ColorRGBA BLUE  = {0,   0,   255, 255};
    constexpr ColorRGBA WHITE = {255, 255, 255, 255};
}

constexpr std::array<Colors::ColorRGBA, 8> TEST_IMAGE0_ROW_PATTERN = {
    Colors::RED,   Colors::RED,   // Pair 1
    Colors::GREEN, Colors::GREEN, // Pair 2
    Colors::BLUE,  Colors::BLUE,  // Pair 3
    Colors::WHITE, Colors::WHITE  // Pair 4
};

constexpr std::size_t WIDTH = 8;
constexpr std::size_t HEIGHT = 8;

constexpr auto generate_rgba_data() {
    std::array<std::uint8_t, WIDTH * HEIGHT * 4> data{};
    for (std::size_t y = 0; y < HEIGHT; ++y) {
        for (std::size_t x = 0; x < WIDTH; ++x) {
            const auto& color = TEST_IMAGE0_ROW_PATTERN[x];
            const std::size_t i = (y * WIDTH + x) * 4;
            data[i + 0] = color.r; data[i + 1] = color.g; data[i + 2] = color.b; data[i + 3] = color.a;
        }
    }
    return data;
}

// 2. RGBからYUVへ変換するヘルパー関数 (4つのバリエーション)
namespace YUVConverters {
    struct ColorYUV { std::uint8_t y, u, v; };
    auto clamp = [](int val) -> std::uint8_t {
        return static_cast<std::uint8_t>(val < 0 ? 0 : (val > 255 ? 255 : val));
    };

    // Limited Range / Rec. 709
    constexpr ColorYUV rgb_to_yuv_limited_709(const Colors::ColorRGBA& c) {
        const int y = ( ( 47 * c.r + 157 * c.g +  16 * c.b + 128) >> 8) + 16;
        const int u = ( (-26 * c.r -  87 * c.g + 113 * c.b + 128) >> 8) + 128;
        const int v = ( (113 * c.r - 102 * c.g -  11 * c.b + 128) >> 8) + 128;
        return { clamp(y), clamp(u), clamp(v) };
    }

    // Limited Range / Rec. 601
    constexpr ColorYUV rgb_to_yuv_limited_601(const Colors::ColorRGBA& c) {
        const int y = ( ( 66 * c.r + 129 * c.g +  25 * c.b + 128) >> 8) + 16;
        const int u = ( (-38 * c.r -  74 * c.g + 112 * c.b + 128) >> 8) + 128;
        const int v = ( (112 * c.r -  94 * c.g -  18 * c.b + 128) >> 8) + 128;
        return { clamp(y), clamp(u), clamp(v) };
    }

    // Full Range / Rec. 709
    constexpr ColorYUV rgb_to_yuv_full_709(const Colors::ColorRGBA& c) {
        const int y = ( 54 * c.r + 183 * c.g +  18 * c.b + 128) >> 8;
        const int u = (-29 * c.r -  99 * c.g + 128 * c.b + 128) >> 8;
        const int v = (128 * c.r - 107 * c.g -  21 * c.b + 128) >> 8;
        return { clamp(y), clamp(u + 128), clamp(v + 128) };
    }

    // Full Range / Rec. 601
    constexpr ColorYUV rgb_to_yuv_full_601(const Colors::ColorRGBA& c) {
        const int y = ( 77 * c.r + 150 * c.g +  29 * c.b + 128) >> 8;
        const int u = (-43 * c.r -  85 * c.g + 128 * c.b + 128) >> 8;
        const int v = (128 * c.r - 107 * c.g -  21 * c.b + 128) >> 8;
        return { clamp(y), clamp(u + 128), clamp(v + 128) };
    }
}

// 3. RGBA画像から各フォーマットのUYVY画像を生成する関数
template<auto ConverterFunc>
constexpr auto generate_uyvy_data() {
    std::array<std::uint8_t, WIDTH * HEIGHT * 2> uyvy_data{};
    for (std::size_t y = 0; y < HEIGHT; ++y) {
        for (std::size_t x = 0; x < WIDTH; x += 2) {
            const auto yuv0 = ConverterFunc(TEST_IMAGE0_ROW_PATTERN[x]);
            const auto yuv1 = ConverterFunc(TEST_IMAGE0_ROW_PATTERN[x + 1]);

            const std::uint8_t u_avg = (static_cast<unsigned int>(yuv0.u) + yuv1.u + 1) / 2;
            const std::uint8_t v_avg = (static_cast<unsigned int>(yuv0.v) + yuv1.v + 1) / 2;

            const std::size_t i = (y * WIDTH + x) * 2;
            uyvy_data[i + 0] = u_avg;   uyvy_data[i + 1] = yuv0.y;
            uyvy_data[i + 2] = v_avg;   uyvy_data[i + 3] = yuv1.y;
        }
    }
    return uyvy_data;
}

// 4. 最終的なテストデータを定数として宣言
constexpr auto EXPECTED_RGBA_DATA = generate_rgba_data();
constexpr auto INPUT_UYVY_LIMITED_709_DATA = generate_uyvy_data<YUVConverters::rgb_to_yuv_limited_709>();
constexpr auto INPUT_UYVY_LIMITED_601_DATA = generate_uyvy_data<YUVConverters::rgb_to_yuv_limited_601>();
constexpr auto INPUT_UYVY_FULL_709_DATA = generate_uyvy_data<YUVConverters::rgb_to_yuv_full_709>();
constexpr auto INPUT_UYVY_FULL_601_DATA = generate_uyvy_data<YUVConverters::rgb_to_yuv_full_601>();

} // namespace TestData


// テスト対象となるクラスのリストを定義
using namespace kaito_tokyo::obs_backgroundremoval_lite;
using Implementations = ::testing::Types<
    UyvyLimitedRec709FrameExtractor,
    UyvyFullRec709FrameExtractor,
    UyvyLimitedRec601FrameExtractor,
    UyvyFullRec601FrameExtractor
>;

// 型付けテストフィクスチャを定義
template <typename T>
class UyvyFrameExtractorTest : public ::testing::Test {};

TYPED_TEST_SUITE(UyvyFrameExtractorTest, Implementations);

// 型付けテストケースを定義
TYPED_TEST(UyvyFrameExtractorTest, ConvertsKnownColorsCorrectly) {
    // --- 1. 準備 (Arrange) ---
    using ExtractorType = TypeParam;

    const std::size_t width = TestData::WIDTH;
    const std::size_t height = TestData::HEIGHT;
    const std::size_t total_pixels = width * height;
    const std::size_t linesize = width * 2;

    // TypeParamの型に応じて、テストで使用する入力データをコンパイル時に選択する
    const void* input_data = nullptr;
    if constexpr (std::is_same_v<ExtractorType, UyvyLimitedRec709FrameExtractor>) {
        input_data = TestData::INPUT_UYVY_LIMITED_709_DATA.data();
    } else if constexpr (std::is_same_v<ExtractorType, UyvyFullRec709FrameExtractor>) {
        input_data = TestData::INPUT_UYVY_FULL_709_DATA.data();
    } else if constexpr (std::is_same_v<ExtractorType, UyvyLimitedRec601FrameExtractor>) {
        input_data = TestData::INPUT_UYVY_LIMITED_601_DATA.data();
    } else if constexpr (std::is_same_v<ExtractorType, UyvyFullRec601FrameExtractor>) {
        input_data = TestData::INPUT_UYVY_FULL_601_DATA.data();
    }

    const void* input_planes[8] = { input_data };

    // 出力先バッファ
    std::vector<float> r_channel(total_pixels);
    std::vector<float> g_channel(total_pixels);
    std::vector<float> b_channel(total_pixels);

    // テスト対象のインスタンス
    ExtractorType extractor;

    // --- 2. 実行 (Act) ---
    extractor(r_channel.data(), g_channel.data(), b_channel.data(), input_planes, width, height, linesize);

    // --- 3. 検証 (Assert) ---
    const float tolerance = 2.0f / 255.0f;

    for (std::size_t i = 0; i < total_pixels; ++i) {
        const float expected_r = static_cast<float>(TestData::EXPECTED_RGBA_DATA[i * 4 + 0]) / 255.0f;
        const float expected_g = static_cast<float>(TestData::EXPECTED_RGBA_DATA[i * 4 + 1]) / 255.0f;
        const float expected_b = static_cast<float>(TestData::EXPECTED_RGBA_DATA[i * 4 + 2]) / 255.0f;

        ASSERT_NEAR(expected_r, r_channel[i], tolerance) << "Pixel " << i << " (R channel) for " << typeid(ExtractorType).name();
        ASSERT_NEAR(expected_g, g_channel[i], tolerance) << "Pixel " << i << " (G channel) for " << typeid(ExtractorType).name();
        ASSERT_NEAR(expected_b, b_channel[i], tolerance) << "Pixel " << i << " (B channel) for " << typeid(ExtractorType).name();
    }
}
