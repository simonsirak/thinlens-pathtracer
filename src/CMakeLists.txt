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
# add_subdirectory("renderer")

# include stuff we wanna include for processing of THIS 
# CMakeLists.txt
include_directories("${PROJECT_SOURCE_DIR}/includes/glm")
include_directories("${PROJECT_SOURCE_DIR}/includes/own")
include_directories("${PROJECT_SOURCE_DIR}/includes/other")
include_directories("${PROJECT_SOURCE_DIR}/camera")

find_package (SDL)

if ( NOT SDL_FOUND )
   message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )

if(SDL_FOUND)
    include_directories(
		${SDL_INCLUDE_DIR}
	)
    # add the executable
    add_executable(ThinLensDebug renderer/raytracer.cpp)
    target_link_libraries(ThinLensDebug Camera ${SDL_LIBRARY})
endif(SDL_FOUND) 

add_executable(ThinLensRender renderer/pathtracer.cpp)
target_link_libraries(ThinLensRender Camera)

