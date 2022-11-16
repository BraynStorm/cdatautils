cmake_minimum_required(VERSION 3.24)

project(cdatautils C CXX)

option(CDATAUTILS_DOWNLOAD_DEPS "Allow cdatautils to download additional dependencies" ON)

find_package(Catch2 QUIET)

if(NOT Catch2_FOUND AND CDATAUTILS_DOWNLOAD_DEPS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        GIT_TAG v3.1.0
        OVERRIDE_FIND_PACKAGE
    )
    find_package(Catch2)
elseif(NOT CDATAUTILS_DOWNLOAD_DEPS)
    find_package(Catch2)
endif()

add_subdirectory(cdatautils)
