add_library(CurlHelper INTERFACE)
target_include_directories(CurlHelper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CurlHelper INTERFACE CURL::libcurl)
target_sources(
  CurlHelper
  PRIVATE
    KaitoTokyo/CurlHelper/CurlHandle.hpp
    KaitoTokyo/CurlHelper/CurlReadCallback.hpp
    KaitoTokyo/CurlHelper/CurlSlistHandle.hpp
    KaitoTokyo/CurlHelper/CurlUrlHandle.hpp
    KaitoTokyo/CurlHelper/CurlUrlSearchParams.hpp
    KaitoTokyo/CurlHelper/CurlWriteCallback.hpp
)
