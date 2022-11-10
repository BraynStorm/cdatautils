cmake_minimum_required(VERSION 3.24)

project(example C)

# Don't download it if we're build cvector right now.
if(NOT ${CMAKE_PROJECT_NAME} STREQUAL "cvector")
    # No need to build tests when just using the library.
    set(CVECTOR_TESTS OFF)

    # Fetch the library from GitHub.
    FetchContent_Declare(
        cvector
        GIT_REPOSITORY https://github.com/braynstorm/cvector
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        GIT_TAG v/1
        OVERRIDE_FIND_PACKAGE
    )

    # Use it.
    find_package(cvector)
endif()

add_executable(example_basic example_basic.c)
add_executable(example_custom_types example_custom_types.c)

target_link_libraries(example_basic cvector)
target_link_libraries(example_custom_types cvector)
