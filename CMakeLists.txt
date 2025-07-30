cmake_minimum_required(VERSION 3.15)
project(OpenGLDemo LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)


# Find all source files
file(GLOB SOURCES "src/*.cpp" "src/*.h")

# Create executable
add_executable(OpenGLDemo ${SOURCES} "lib/tgaimage.cpp" "include/model.h" "src/model.cpp")

# Set the output directory
set_target_properties(OpenGLDemo PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
