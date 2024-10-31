include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.3
)
FetchContent_MakeAvailable(Catch2)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
file(GLOB TEST_FILES "./*.cc")
add_executable(${PROJECT_NAME}_tests ${TEST_FILES} ${SRC_DIR}/music_list.cc ${SRC_DIR}/runtime.cc ${SRC_DIR}/utils.cc)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE Catch2::Catch2WithMain fmt::fmt)
target_include_directories(${PROJECT_NAME}_tests PRIVATE ${SRC_DIR})
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_tests)