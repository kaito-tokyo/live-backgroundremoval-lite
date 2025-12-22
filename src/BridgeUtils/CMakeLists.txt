add_library(BridgeUtils INTERFACE)
target_include_directories(BridgeUtils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BridgeUtils INTERFACE OBS::libobs Logger)
target_sources(BridgeUtils PRIVATE AsyncTextureReader.hpp GsUnique.hpp ObsLogger.hpp ObsUnique.hpp)
