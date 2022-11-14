cmake_minimum_required(VERSION 3.24)

project(example C)

# Don't download it if we're build cvector right now.
if(NOT ${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
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
    find_package(cdatautils)
endif()

add_executable(cdatautils-example-basic example_basic.c)
add_executable(cdatautils-example-custom_types example_custom_types.c)

target_link_libraries(cdatautils-example-basic cdatautils::vector)
target_link_libraries(cdatautils-example-custom_types cdatautils::vector)
