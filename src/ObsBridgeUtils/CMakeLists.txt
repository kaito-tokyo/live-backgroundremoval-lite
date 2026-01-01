add_library(ObsBridgeUtils INTERFACE)
target_include_directories(ObsBridgeUtils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ObsBridgeUtils INTERFACE fmt::fmt OBS::libobs Logger)
target_sources(
  ObsBridgeUtils
  PRIVATE
    KaitoTokyo/ObsBridgeUtils/AsyncTextureReader.hpp
    KaitoTokyo/ObsBridgeUtils/GsUnique.hpp
    KaitoTokyo/ObsBridgeUtils/ObsLogger.hpp
    KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp
)
