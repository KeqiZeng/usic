cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

project(usic)
set(SOURCES src/usic.cc src/client.cc src/commands.cc src/core.cc src/music_list.cc src/runtime.cc src/server.cc src/utils.cc)
add_executable(${PROJECT_NAME} ${SOURCES})

include(FetchContent)

# ## dependences ###
# miniaudio
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio
  GIT_TAG 0.11.21
)
FetchContent_MakeAvailable(miniaudio)
target_include_directories(${PROJECT_NAME} PRIVATE ${miniaudio_SOURCE_DIR})

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(${PROJECT_NAME} fmt::fmt)

# ## test ###
option(BUILD_TESTING "Build test targets" OFF)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()