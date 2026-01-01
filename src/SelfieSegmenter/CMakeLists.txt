add_library(SelfieSegmenter STATIC)
target_include_directories(SelfieSegmenter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SelfieSegmenter PUBLIC ncnn::ncnn Memory)
target_sources(
  SelfieSegmenter
  PRIVATE
    KaitoTokyo/SelfieSegmenter/BoundingBox.cpp
    KaitoTokyo/SelfieSegmenter/BoundingBox.hpp
    KaitoTokyo/SelfieSegmenter/ISelfieSegmenter.hpp
    KaitoTokyo/SelfieSegmenter/MaskBuffer.hpp
    KaitoTokyo/SelfieSegmenter/NcnnSelfieSegmenter.hpp
    KaitoTokyo/SelfieSegmenter/NullSelfieSegmenter.hpp
    KaitoTokyo/SelfieSegmenter/ShapeConverter.cpp
    KaitoTokyo/SelfieSegmenter/ShapeConverter.hpp
)
