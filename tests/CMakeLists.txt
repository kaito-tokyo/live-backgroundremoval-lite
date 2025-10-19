include(GoogleTest)

set(CMAKE_SKIP_RPATH FALSE)

# SelfieSegmenter_test
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

if(COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH)
  # CoreMLSelfieSegmenter_test
  add_executable(CoreMLSelfieSegmenter_test SelfieSegmenter/CoreMLSelfieSegmenter_test.cpp)
  target_compile_definitions(
    CoreMLSelfieSegmenter_test
    PRIVATE DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\" TESTS_DIR=\"${CMAKE_SOURCE_DIR}/tests\" NOMINMAX
  )
  target_link_libraries(
    CoreMLSelfieSegmenter_test
    PRIVATE GTest::gtest_main SelfieSegmenter OpenCV::opencv_core OpenCV::opencv_imgproc OpenCV::opencv_imgcodecs
  )
  target_link_libraries(SelfieSegmenter PRIVATE "${COREML_SELFIE_SEGMENTER_FRAMEWORK_PATH}")
  set_target_properties(CoreMLSelfieSegmenter_test
      PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
  )
  gtest_discover_tests(CoreMLSelfieSegmenter_test DISCOVERY_MODE PRE_TEST)
endif()

# UpdateChecker_test
add_executable(UpdateChecker_test UpdateChecker/UpdateChecker_test.cpp)
target_include_directories(UpdateChecker_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main UpdateChecker)
target_compile_definitions(UpdateChecker_test PRIVATE NOMINMAX)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)
