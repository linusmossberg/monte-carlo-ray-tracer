cmake_minimum_required(VERSION 3.10)

project(monte-carlo-ray-tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG(-O3 COMPILER_SUPPORTS_O3)
if(COMPILER_SUPPORTS_O3)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

CHECK_CXX_COMPILER_FLAG(-g COMPILER_SUPPORTS_G)
if(COMPILER_SUPPORTS_G)
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

include_directories(${PROJECT_SOURCE_DIR}/lib/glm/)
include_directories(${PROJECT_SOURCE_DIR}/lib/nlohmann/)

file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE headers ${PROJECT_SOURCE_DIR}/source/*.hpp)

add_executable(${PROJECT_NAME} ${sources} ${headers})

target_link_libraries(${PROJECT_NAME} Threads::Threads)
