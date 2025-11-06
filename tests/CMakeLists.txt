include(GoogleTest)

set(TEST_LIST "")

add_executable(NcnnSelfieSegmenter_test SelfieSegmenter/NcnnSelfieSegmenter_test.cpp)
target_link_libraries(
  NcnnSelfieSegmenter_test
  PRIVATE GTest::gtest_main SelfieSegmenter OpenCV::opencv_core OpenCV::opencv_imgproc OpenCV::opencv_imgcodecs
)
list(APPEND TEST_LIST NcnnSelfieSegmenter_test)

# UpdateChecker_test
add_executable(UpdateChecker_test UpdateChecker/UpdateChecker_test.cpp)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main UpdateChecker)
list(APPEND TEST_LIST UpdateChecker_test)

foreach(TEST_NAME IN LISTS TEST_LIST)
  set_target_properties(
    ${TEST_NAME}
    PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
      XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "${DERIVED_DATA_PATH}"
  )
  target_compile_definitions(
    ${TEST_NAME}
    PRIVATE DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\" TESTS_DIR=\"${CMAKE_SOURCE_DIR}/tests\" NOMINMAX
  )
  gtest_discover_tests(${TEST_NAME} DISCOVERY_MODE PRE_TEST)
endforeach()
