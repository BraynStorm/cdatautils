
set(BENCHMARK_DOWNLOAD_DEPENDENCIES CDATAUTILS_DOWNLOAD_DEPS)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_SHARED OFF)
set(HAVE_STD_REGEX ON)
set(CMAKE_CXX_STANDARD 17)

if(CDATAUTILS_DOWNLOAD_DEPS)
    find_package(benchmark QUIET)

    if(NOT benchmark_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark
            GIT_SHALLOW ON
            GIT_PROGRESS ON
            GIT_TAG v1.7.0
            OVERRIDE_FIND_PACKAGE
        )
    endif()
endif()

find_package(benchmark REQUIRED)

add_executable(
    cdatautils-ringbuffer-benchmark
    ringbuffer.cpp
)

target_link_libraries(cdatautils-ringbuffer-benchmark PUBLIC benchmark::benchmark ringbuffer)