include(GoogleTest)

add_executable(SelfieSegmenter_test SelfieSegmenter/SelfieSegmenter_test.cpp)
target_include_directories(SelfieSegmenter_test PRIVATE ${Stb_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(SelfieSegmenter_test PRIVATE GTest::gtest_main ncnn)
gtest_discover_tests(SelfieSegmenter_test DISCOVERY_MODE PRE_TEST)

