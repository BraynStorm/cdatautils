cmake_minimum_required(VERSION 3.24)

project(cvector C CXX)

option(CVECTOR_FORCE_STATIC "Force <cvector> to be built as a static library")
option(CVECTOR_TESTS "Build cvector-test" ON)
option(CVECTOR_ASSERTS "Build cvector with asserts (debug only)" ON)
option(CVECTOR_DOWNLOAD_DEPS "Allow <cvector> to be download additional dependencies" ON)

set(CVECTOR_SOURCES cvector.c)
set(CVECTOR_HEADERS cvector.h)

if(BUILD_SHARED_LIBS AND NOT CVECTOR_FORCE_STATIC)
    add_library(cvector SHARED ${CVECTOR_SOURCES})
else()
    add_library(cvector STATIC ${CVECTOR_SOURCES})
endif()

if(MSVC)
    target_link_options(cvector PRIVATE "/NATVIS:cvector.natvis")
endif()

target_include_directories(cvector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(
    cvector
    PROPERTIES
    PUBLIC_HEADER ${CVECTOR_HEADERS}
    C_STANDARD 23
    C_STANDARD_REQUIRED 11
    C_EXTENSIONS OFF
)

if(CVECTOR_ASSERTS)
    target_compile_definitions(cvector PRIVATE VECTOR_USE_ASSERT)
endif()

if(CVECTOR_TESTS)
    find_package(Catch2 QUIET)

    if(NOT Catch2_FOUND AND CVECTOR_DOWNLOAD_DEPS)
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
    elseif(NOT CVECTOR_DOWNLOAD_DEPS)
        find_package(Catch2)
    endif()

    if(Catch2_FOUND)
        include(CTest)
        include(Catch)

        add_executable(cvector-test cvector-test.cpp)
        target_link_libraries(cvector-test PUBLIC cvector Catch2::Catch2WithMain)

        set_target_properties(
            cvector-test
            PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED 17
            CXX_EXTENSIONS OFF
        )
        catch_discover_tests(cvector-test)
    else()
        message("[cvector - INFO] Catch2 was not found. The cvector-test target will not be created.")
    endif()
endif()

install(
    TARGETS cvector
    CONFIGURATIONS Debug
    RUNTIME
    LIBRARY
    PUBLIC_HEADER
)
