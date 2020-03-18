cmake_minimum_required(VERSION 3.10)

project(monte-carlo-ray-tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/lib/glm/)
include_directories(${PROJECT_SOURCE_DIR}/lib/nlohmann/)

file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE headers ${PROJECT_SOURCE_DIR}/source/*.hpp)

add_executable(${PROJECT_NAME} ${sources} ${headers})