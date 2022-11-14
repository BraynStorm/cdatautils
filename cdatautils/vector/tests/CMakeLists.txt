find_package(Catch2 QUIET)

if(Catch2_FOUND)
    include(CTest)
    include(Catch)

    add_executable(cdatautils-vector-test vector-test.cpp)

    target_link_libraries(cdatautils-vector-test PUBLIC vector Catch2::Catch2WithMain)

    set_target_properties(
        cdatautils-vector-test
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED 17
        CXX_EXTENSIONS OFF
    )

    catch_discover_tests(cdatautils-vector-test)
else()
    message("[cdatautils-vector - INFO] Catch2 was not found. The test targets will not be created.")
endif()