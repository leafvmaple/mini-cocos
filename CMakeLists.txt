# Minimal cocos-style 2D engine for learning (GLFW + OpenGL 3.3 Core).
# Build (from this directory):
#   cmake -B build
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(mini-cocos LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(mini-cocos
  src/main.cpp
  src/opengl_loader.cpp
  src/Director.cpp
  src/Node.cpp
  src/Sprite.cpp
)

target_include_directories(mini-cocos PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

target_link_libraries(mini-cocos PRIVATE glfw OpenGL::GL)

if(WIN32)
  target_compile_definitions(mini-cocos PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
  target_compile_options(mini-cocos PRIVATE /utf-8)
endif()
