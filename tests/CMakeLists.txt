include(GoogleTest)

set(TEST_LIST "")

add_executable(NcnnSelfieSegmenter_test
SelfieSegmenter/NcnnSelfieSegmenter_test.cpp
../src/LiveBackgroundRemovalLite/MainFIlter/mediapipe_selfie_segmentation_landscape_int8_ncnn_bin.c
../src/LiveBackgroundRemovalLite/MainFIlter/mediapipe_selfie_segmentation_landscape_int8_ncnn_param.cpp
)
target_link_libraries(NcnnSelfieSegmenter_test PRIVATE GTest::gtest_main SelfieSegmenter stb::stb)
list(APPEND TEST_LIST NcnnSelfieSegmenter_test)

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
    PRIVATE DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../data\" TESTS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../tests\" NOMINMAX
  )
  gtest_discover_tests(${TEST_NAME} DISCOVERY_MODE PRE_TEST)
endforeach()
