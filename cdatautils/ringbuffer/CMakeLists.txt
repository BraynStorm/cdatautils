option(CDATAUTILS_RINGBUFFER_ASSERTS "Build cdatautils/ringbuffer with asserts (debug only)." ON)
option(CDATAUTILS_RINGBUFFER_TESTS "Enable cdatautils/ringbuffer tests." OFF)
option(CDATAUTILS_RINGBUFFER_BENCHMARKS "Enable cdatautils/ringbuffer benchmarks." OFF)

add_library(ringbuffer STATIC src/ringbuffer.c)

add_library(cdatautils::ringbuffer ALIAS ringbuffer)

if(MSVC)
    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(ringbuffer PRIVATE /Wall /WX)
    endif()
else()
    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(ringbuffer PRIVATE -Weverything -Werror -Wno-unsafe-buffer-usage)
    endif()
endif()

target_include_directories(ringbuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(
    ringbuffer
    INTERFACE
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/cdatautils
)

if(CDATAUTILS_RINGBUFFER_TESTS)
    add_subdirectory(tests)
endif()

if(CDATAUTILS_RINGBUFFER_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(CDATAUTILS_RINGBUFFER_ASSERTS)
    target_compile_definitions(ringbuffer PRIVATE CDATAUTILS_RINGBUFFER_USE_ASSERT=1)
endif()

set_target_properties(
    ringbuffer
    PROPERTIES
    COMPILE_WARNING_AS_ERROR ON
    C_STANDARD 23
    C_STANDARD_REQUIRED 11
    C_EXTENSIONS OFF
)

install(
    TARGETS ringbuffer
    CONFIGURATIONS ${CMAKE_BUILD_TYPE}
    RUNTIME
    LIBRARY
    FILE_SET headers
)