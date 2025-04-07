cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

project(usic)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
file(GLOB SOURCES ${SOURCE_DIR}/*.cc)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external)

### test ###
option(BUILD_TESTING "Build test targets" OFF)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
