# zocos: a small cocos2d-x–style 2D engine for learning (GLFW + OpenGL 3.3 Core).
# Build (from this directory):
#   cmake -B build
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(zocos LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
# Helps clangd / IDE: Ninja & Makefile generators emit build/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ZOCOS_RENDER_API "VULKAN" CACHE STRING "Render backend (VULKAN or OPENGL)")
set_property(CACHE ZOCOS_RENDER_API PROPERTY STRINGS VULKAN OPENGL)

string(TOUPPER "${ZOCOS_RENDER_API}" ZOCOS_RENDER_API_UPPER)
if(NOT ZOCOS_RENDER_API_UPPER STREQUAL "VULKAN" AND
   NOT ZOCOS_RENDER_API_UPPER STREQUAL "OPENGL")
  message(FATAL_ERROR "ZOCOS_RENDER_API must be VULKAN or OPENGL")
endif()

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

if(ZOCOS_RENDER_API_UPPER STREQUAL "OPENGL")
  find_package(OpenGL REQUIRED)
elseif(ZOCOS_RENDER_API_UPPER STREQUAL "VULKAN")
  find_package(Vulkan QUIET)
  if(Vulkan_FOUND)
    set(ZOCOS_VULKAN_LINK_TARGET Vulkan::Vulkan)
    message(STATUS "Vulkan SDK found. Using system Vulkan package.")
  else()
    message(STATUS "Vulkan SDK not found. Fetching Vulkan-Loader and dependencies...")

    if(WIN32)
      # update_deps.py captures subprocess text using the process default encoding.
      # Force UTF-8 mode to avoid cp936/gbk decode failures in non-ASCII output.
      set(ENV{PYTHONUTF8} "1")
      set(ENV{PYTHONIOENCODING} "utf-8")
    endif()

    # Vulkan-Loader can pull Vulkan-Headers automatically with UPDATE_DEPS.
    set(UPDATE_DEPS ON CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_WERROR OFF CACHE BOOL "" FORCE)
    set(LOADER_CODEGEN OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
      vulkan_loader
      GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
      GIT_TAG        v1.4.313
    )
    FetchContent_MakeAvailable(vulkan_loader)

    if(TARGET Vulkan::Loader)
      set(ZOCOS_VULKAN_LINK_TARGET Vulkan::Loader)
    elseif(TARGET vulkan)
      set(ZOCOS_VULKAN_LINK_TARGET vulkan)
    else()
      message(FATAL_ERROR "Vulkan-Loader was fetched but no loader target was exported.")
    endif()
  endif()
endif()

message(STATUS "zocos render backend: ${ZOCOS_RENDER_API_UPPER}")

file(GLOB_RECURSE ZOCOS_COMMON_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(FILTER ZOCOS_COMMON_SOURCES EXCLUDE REGEX "/src/platform/opengl/.*\\.cpp$")
list(FILTER ZOCOS_COMMON_SOURCES EXCLUDE REGEX "/src/platform/vulkan/.*\\.cpp$")

if(ZOCOS_RENDER_API_UPPER STREQUAL "OPENGL")
  file(GLOB_RECURSE ZOCOS_BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/opengl/*.cpp"
  )
elseif(ZOCOS_RENDER_API_UPPER STREQUAL "VULKAN")
  file(GLOB_RECURSE ZOCOS_BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/vulkan/*.cpp"
  )
endif()

set(ZOCOS_SOURCES ${ZOCOS_COMMON_SOURCES} ${ZOCOS_BACKEND_SOURCES})

add_executable(zocos ${ZOCOS_SOURCES})

target_include_directories(zocos PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

target_link_libraries(zocos PRIVATE glfw lua_static)

if(ZOCOS_RENDER_API_UPPER STREQUAL "OPENGL")
  target_link_libraries(zocos PRIVATE OpenGL::GL)
elseif(ZOCOS_RENDER_API_UPPER STREQUAL "VULKAN")
  if(NOT ZOCOS_VULKAN_LINK_TARGET)
    message(FATAL_ERROR "No Vulkan link target resolved for Vulkan backend.")
  endif()

  target_link_libraries(zocos PRIVATE ${ZOCOS_VULKAN_LINK_TARGET})

  if(TARGET Vulkan::Headers)
    target_link_libraries(zocos PRIVATE Vulkan::Headers)
  elseif(Vulkan_FOUND AND Vulkan_INCLUDE_DIRS)
    target_include_directories(zocos PRIVATE ${Vulkan_INCLUDE_DIRS})
  endif()

  if(DEFINED VULKAN_HEADERS_INSTALL_DIR AND
     EXISTS "${VULKAN_HEADERS_INSTALL_DIR}/include/vulkan/vulkan.h")
    target_include_directories(zocos PRIVATE "${VULKAN_HEADERS_INSTALL_DIR}/include")
  endif()
endif()

add_custom_command(TARGET zocos POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
    "$<TARGET_FILE_DIR:zocos>/scripts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
    "$<TARGET_FILE_DIR:zocos>/fonts"
)

if(WIN32)
  target_compile_definitions(zocos PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
  target_compile_options(zocos PRIVATE /utf-8)
endif()
