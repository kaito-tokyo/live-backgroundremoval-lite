include(GoogleTest)
add_executable(Stub_test Stub_test.cpp)
target_link_libraries(Stub_test PRIVATE onnxruntime::onnxruntime GTest::gtest_main)
set_target_properties(Stub_test PROPERTIES
    OSX_ARCHITECTURES "arm64"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
)
gtest_discover_tests(Stub_test DISCOVERY_MODE PRE_TEST)
