include(GoogleTest)

add_executable(
  SelfieSegmenter_test
  SelfieSegmenter/SelfieSegmenter_test.cpp
)
target_include_directories(
  SelfieSegmenter_test
  PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/SelfieSegmenter
)
target_compile_definitions(
  SelfieSegmenter_test
  PRIVATE DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\" TESTS_DIR=\"${CMAKE_SOURCE_DIR}/tests\" NOMINMAX
)

target_link_libraries(
  SelfieSegmenter_test
  PRIVATE GTest::gtest_main SelfieSegmenter
)
gtest_discover_tests(SelfieSegmenter_test DISCOVERY_MODE PRE_TEST)

add_executable(UpdateChecker_test UpdateChecker/UpdateChecker_test.cpp)
target_include_directories(UpdateChecker_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main CURL::libcurl)
target_compile_definitions(UpdateChecker_test PRIVATE NOMINMAX)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)
