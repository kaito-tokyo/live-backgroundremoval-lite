include(GoogleTest)

# # SelfieSegmenter_test
add_executable(SelfieSegmenter_test SelfieSegmenter/SelfieSegmenter_test.cpp)
target_compile_definitions(
  SelfieSegmenter_test
  PRIVATE DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\" TESTS_DIR=\"${CMAKE_SOURCE_DIR}/tests\" NOMINMAX
)
target_link_libraries(
  SelfieSegmenter_test
  PRIVATE GTest::gtest_main SelfieSegmenter OpenCV::opencv_core OpenCV::opencv_imgproc OpenCV::opencv_imgcodecs
)
gtest_discover_tests(SelfieSegmenter_test DISCOVERY_MODE PRE_TEST)

# UpdateChecker_test
add_executable(UpdateChecker_test UpdateChecker/UpdateChecker_test.cpp)
target_include_directories(UpdateChecker_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main UpdateChecker)
target_compile_definitions(UpdateChecker_test PRIVATE NOMINMAX)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)
