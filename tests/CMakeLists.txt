include(GoogleTest)
add_executable(Stub_test Stub_test.cpp)
target_link_libraries(Stub_test PRIVATE GTest::gtest_main ncnn)
gtest_discover_tests(Stub_test DISCOVERY_MODE PRE_TEST)
