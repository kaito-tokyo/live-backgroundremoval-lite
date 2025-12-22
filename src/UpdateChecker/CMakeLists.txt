add_library(UpdateChecker INTERFACE)
target_include_directories(UpdateChecker INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UpdateChecker INTERFACE CURL::libcurl)
target_sources(UpdateChecker PRIVATE UpdateChecker.hpp)
