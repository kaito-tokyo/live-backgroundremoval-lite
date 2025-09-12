include(GoogleTest)
add_executable(Stub_test Stub_test.mm)
target_link_libraries(Stub_test PRIVATE opencv_core opencv_imgproc opencv_imgcodecs GTest::gtest_main)
gtest_discover_tests(Stub_test DISCOVERY_MODE PRE_TEST)
