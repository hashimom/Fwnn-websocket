cmake_minimum_required(VERSION 2.8)
PROJECT(ws-fwnn　C)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(server)
add_subdirectory(lib/cwebsocket)
add_subdirectory(lib/easyfwnn)
add_subdirectory(lib/aoi)
