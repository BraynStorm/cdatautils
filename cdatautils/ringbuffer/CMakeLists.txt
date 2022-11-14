option(CDATAUTILS_RINGBUFFER_ASSERTS "Build cdatautils/ringbuffer with asserts (debug only)." ON)
option(CDATAUTILS_RINGBUFFER_TESTS "Enable cdatautils/ringbuffer tests." OFF)
option(CDATAUTILS_RINGBUFFER_BENCHMARKS "Enable cdatautils/ringbuffer benchmarks." OFF)

add_library(ringbuffer STATIC src/ringbuffer.c)

add_library(cdatautils::ringbuffer ALIAS ringbuffer)

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