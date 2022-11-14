
option(CDATAUTILS_VECTOR_TESTS "Build cdatautils/vector tests" OFF)
option(CDATAUTILS_VECTOR_ASSERTS "Build cdatautils/vector with asserts (debug only)" ON)
option(CDATAUTILS_VECTOR_EXAMPLES "Build cdatautils/vector examples" OFF)

add_library(vector STATIC src/vector.c)
add_library(cdatautils::vector ALIAS vector)

if(MSVC)
    target_link_options(vector PRIVATE "/NATVIS:cdatautils-vector.natvis")
endif()

target_include_directories(vector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(
    vector
    INTERFACE
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/cdatautils
)

if(CDATAUTILS_VECTOR_ASSERTS)
    target_compile_definitions(vector PRIVATE CDATAUTILS_VECTOR_USE_ASSERT=1)
endif()

if(CDATAUTILS_VECTOR_TESTS)
    add_subdirectory(tests)
endif()

if(CDATAUTILS_VECTOR_EXAMPLES)
    add_subdirectory(example/cmake-project)
endif()

set_target_properties(
    vector
    PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED 11
    C_EXTENSIONS OFF
)

install(
    TARGETS vector
    CONFIGURATIONS ${CMAKE_BUILD_TYPE}
    RUNTIME
    LIBRARY
    FILE_SET headers
)
install(
    FILES
    cdatautils-vector.natvis
    DESTINATION natvis
)
