cmake_minimum_required(VERSION 3.16)

project(x_muduo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(X_MUDUO_BUILD_SHARED "Build x_muduo as a shared library" OFF)

file(GLOB_RECURSE X_MUDUO_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
)

if(X_MUDUO_BUILD_SHARED)
  add_library(x_muduo SHARED ${X_MUDUO_SOURCES})
else()
  add_library(x_muduo STATIC ${X_MUDUO_SOURCES})
endif()

target_include_directories(x_muduo
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(x_muduo
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

set_target_properties(x_muduo PROPERTIES
  OUTPUT_NAME x_muduo
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON
)
