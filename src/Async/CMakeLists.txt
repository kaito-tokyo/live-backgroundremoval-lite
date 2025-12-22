add_library(Async INTERFACE)
target_include_directories(Async INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Async INTERFACE SymmetricTransfer.hpp Task.hpp TaskStorage.hpp)
