include(GoogleTest)

add_executable(
  SelfieSegmenter_test
  ${CMAKE_SOURCE_DIR}/src/SelfieSegmenter/SelfieSegmenter.cpp
  SelfieSegmenter/SelfieSegmenter_test.cpp
)
target_include_directories(
  SelfieSegmenter_test
  PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/SelfieSegmenter
)
target_compile_definitions(
  SelfieSegmenter_test
  PRIVATE DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\" TESTS_DIR=\"${CMAKE_SOURCE_DIR}/tests\"
)
target_link_libraries(SelfieSegmenter_test PRIVATE GTest::gtest_main ncnn)
gtest_discover_tests(SelfieSegmenter_test DISCOVERY_MODE PRE_TEST)
