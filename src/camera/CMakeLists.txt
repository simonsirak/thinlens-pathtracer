include_directories("${PROJECT_SOURCE_DIR}/includes/glm")
include_directories("${PROJECT_SOURCE_DIR}/includes/own")
include_directories("${PROJECT_SOURCE_DIR}/camera")

add_library(Camera camera.cpp projective.cpp perspective.cpp)