# zocos: a small cocos2d-x–style 2D engine for learning (GLFW + OpenGL 3.3 Core).
# Build (from this directory):
#   cmake -B build
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(zocos LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
# Helps clangd / IDE: Ninja & Makefile generators emit build/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

file(GLOB_RECURSE ZOCOS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(zocos ${ZOCOS_SOURCES})

target_include_directories(zocos PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

target_link_libraries(zocos PRIVATE glfw OpenGL::GL)

if(WIN32)
  target_compile_definitions(zocos PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
  target_compile_options(zocos PRIVATE /utf-8)
endif()
