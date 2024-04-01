cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

project(usic)
include_directories(include)
set(SOURCES src/main.cpp src/core.cpp src/commands.cpp src/callback.cpp src/runtime.cpp)
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
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.3
)
FetchContent_MakeAvailable(Catch2)

file(GLOB TEST_FILES "tests/*.cpp")
add_executable(${PROJECT_NAME}_tests ${TEST_FILES} src/runtime.cpp)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE Catch2::Catch2WithMain fmt::fmt)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_tests)
