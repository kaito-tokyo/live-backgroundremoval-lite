include(GoogleTest)
add_executable(Stub_test Stub_test.cpp)
target_link_libraries(Stub_test PRIVATE GTest::gtest_main ncnn opencv_core opencv_imgproc opencv_imgcodecs)
gtest_discover_tests(Stub_test DISCOVERY_MODE PRE_TEST)
