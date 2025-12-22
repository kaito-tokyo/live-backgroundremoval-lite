add_library(SelfieSegmenter STATIC)
target_include_directories(SelfieSegmenter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SelfieSegmenter PUBLIC ncnn::ncnn Memory)
target_sources(
  SelfieSegmenter
  PRIVATE
    ISelfieSegmenter.hpp
    MaskBuffer.hpp
    NcnnSelfieSegmenter.hpp
    NullSelfieSegmenter.hpp
    ShapeConverter.cpp
    ShapeConverter.hpp
)
