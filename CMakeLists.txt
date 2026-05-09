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

FetchContent_Declare(
  lua_src
  URL https://www.lua.org/ftp/lua-5.4.6.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(lua_src)

set(LUA_SOURCE_DIR "${lua_src_SOURCE_DIR}/src")
add_library(lua_static STATIC
  "${LUA_SOURCE_DIR}/lapi.c"
  "${LUA_SOURCE_DIR}/lauxlib.c"
  "${LUA_SOURCE_DIR}/lbaselib.c"
  "${LUA_SOURCE_DIR}/lcode.c"
  "${LUA_SOURCE_DIR}/lcorolib.c"
  "${LUA_SOURCE_DIR}/lctype.c"
  "${LUA_SOURCE_DIR}/ldblib.c"
  "${LUA_SOURCE_DIR}/ldebug.c"
  "${LUA_SOURCE_DIR}/ldo.c"
  "${LUA_SOURCE_DIR}/ldump.c"
  "${LUA_SOURCE_DIR}/lfunc.c"
  "${LUA_SOURCE_DIR}/lgc.c"
  "${LUA_SOURCE_DIR}/linit.c"
  "${LUA_SOURCE_DIR}/liolib.c"
  "${LUA_SOURCE_DIR}/llex.c"
  "${LUA_SOURCE_DIR}/lmathlib.c"
  "${LUA_SOURCE_DIR}/lmem.c"
  "${LUA_SOURCE_DIR}/loadlib.c"
  "${LUA_SOURCE_DIR}/lobject.c"
  "${LUA_SOURCE_DIR}/lopcodes.c"
  "${LUA_SOURCE_DIR}/loslib.c"
  "${LUA_SOURCE_DIR}/lparser.c"
  "${LUA_SOURCE_DIR}/lstate.c"
  "${LUA_SOURCE_DIR}/lstring.c"
  "${LUA_SOURCE_DIR}/lstrlib.c"
  "${LUA_SOURCE_DIR}/ltable.c"
  "${LUA_SOURCE_DIR}/ltablib.c"
  "${LUA_SOURCE_DIR}/ltm.c"
  "${LUA_SOURCE_DIR}/lundump.c"
  "${LUA_SOURCE_DIR}/lutf8lib.c"
  "${LUA_SOURCE_DIR}/lvm.c"
  "${LUA_SOURCE_DIR}/lzio.c"
)
target_include_directories(lua_static PUBLIC "${LUA_SOURCE_DIR}")

find_package(OpenGL REQUIRED)

file(GLOB_RECURSE ZOCOS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(zocos ${ZOCOS_SOURCES})

target_include_directories(zocos PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

target_link_libraries(zocos PRIVATE glfw OpenGL::GL lua_static)

add_custom_command(TARGET zocos POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
    "$<TARGET_FILE_DIR:zocos>/scripts"
)

if(WIN32)
  target_compile_definitions(zocos PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
  target_compile_options(zocos PRIVATE /utf-8)
endif()
