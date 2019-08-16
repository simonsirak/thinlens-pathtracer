cmake_minimum_required (VERSION 2.6)
project (ThinLensPT)

# The version number.
set (ThinLensPT_VERSION_MAJOR 1)
set (ThinLensPT_VERSION_MINOR 0)

# Ensure c++11 is used and use optimization
SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++11 -O3")
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

# Add subdirectory (will invoke CMakeLists.txt in that dir)
# This will recurse into that dir, run its CMakeLists.txt, 
# then proceed here.
add_subdirectory("camera")

# include stuff we wanna include for processing of THIS 
# CMakeLists.txt
include_directories("${PROJECT_SOURCE_DIR}/includes/glm")
include_directories("${PROJECT_SOURCE_DIR}/includes/bmp")
include_directories("${PROJECT_SOURCE_DIR}/camera")
 
# add the executable
add_executable(ThinLensPT main.cpp)
target_link_libraries(ThinLensPT Camera)