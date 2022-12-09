option(CDATAUTILS_HASHMAP_ASSERTS "Build cdatautils/hashmap with asserts (debug only)." ON)
option(CDATAUTILS_HASHMAP_TESTS "Enable cdatautils/hashmap tests." OFF)
option(CDATAUTILS_HASHMAP_BENCHMARKS "Enable cdatautils/hashmap benchmarks." OFF)

add_library(hashmap STATIC src/hashmap.c)

add_library(cdatautils::hashmap ALIAS hashmap)

if(MSVC)
    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(hashmap PRIVATE /Wall /WX)
    endif()
else()
    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(hashmap PRIVATE -Weverything -Werror)
    endif()
endif()

target_include_directories(hashmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(CDATAUTILS_HASHMAP_TESTS)
    add_subdirectory(tests)
endif()

if(CDATAUTILS_HASHMAP_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(CDATAUTILS_HASHMAP_ASSERTS)
    target_compile_definitions(hashmap PRIVATE CDATAUTILS_HASHMAP_USE_ASSERT=1)
endif()

set_target_properties(
    hashmap
    PROPERTIES
    COMPILE_WARNING_AS_ERROR ON
    C_STANDARD 23
    C_STANDARD_REQUIRED 11
    C_EXTENSIONS OFF
)

install(
    TARGETS hashmap
    CONFIGURATIONS ${CMAKE_BUILD_TYPE}
    RUNTIME
    LIBRARY
    FILE_SET headers
)