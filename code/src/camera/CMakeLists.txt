include_directories("${PROJECT_SOURCE_DIR}/include/int")
include_directories("${PROJECT_SOURCE_DIR}/include/ext")

add_library(Camera camera.cpp projective.cpp perspective.cpp)