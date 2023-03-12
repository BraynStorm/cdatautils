add_library(text STATIC src/text.c)
add_library(cdatautils::text ALIAS text)

if(MSVC)
    target_link_options(text PRIVATE "/NATVIS:cdatautils-vector.natvis")

    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(text PRIVATE /Wall /WX)
    endif()
else()
    if(${CMAKE_PROJECT_NAME} STREQUAL "cdatautils")
        target_compile_options(
            text
            PRIVATE
            -Weverything
            -Werror
            -Wno-unused-function
            -Wno-unsafe-buffer-usage
        )
    endif()
endif()

target_include_directories(text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(
    text
    INTERFACE
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/cdatautils
)

if(CDATAUTILS_TEXT_ASSERTS)
    target_compile_definitions(text PRIVATE CDATAUTILS_TEXT_USE_ASSERT=1)
endif()

if(CDATAUTILS_TEXT_TESTS)
    add_subdirectory(tests)
endif()

if(CDATAUTILS_TEXT_EXAMPLES)
    add_subdirectory(example/cmake-project)
endif()

set_target_properties(
    text
    PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED 11
    C_EXTENSIONS OFF
)

install(
    TARGETS text
    CONFIGURATIONS ${CMAKE_BUILD_TYPE}
    RUNTIME
    LIBRARY
    FILE_SET headers
)
install(
    FILES
    cdatautils-text.natvis
    DESTINATION natvis
)
