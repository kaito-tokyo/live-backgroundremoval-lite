include(GoogleTest)
add_executable(Stub_test Stub_test.cpp)
target_link_libraries(
  Stub_test
  PRIVATE GTest::gtest_main ncnn opencv_core opencv_imgproc opencv_imgcodecs
)
gtest_discover_tests(Stub_test DISCOVERY_MODE PRE_TEST)

add_executable(UpdateChecker_test UpdateChecker_test.cpp)
target_link_libraries(
  UpdateChecker_test
  PRIVATE GTest::gtest_main ${CMAKE_PROJECT_NAME}_header cpr::cpr fmt::fmt semver::semver
)
target_compile_definitions(UpdateChecker_test PRIVATE NOMINMAX)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)
