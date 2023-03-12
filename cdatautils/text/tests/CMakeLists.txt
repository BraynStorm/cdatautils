if(Catch2_FOUND)
    include(CTest)
    include(Catch)

    add_executable(cdatautils-text-test text-test.cpp)

    target_link_libraries(cdatautils-text-test PUBLIC text Catch2::Catch2WithMain)

    set_target_properties(
        cdatautils-text-test
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED 17
        CXX_EXTENSIONS OFF
    )

    catch_discover_tests(cdatautils-text-test)
else()
    message("[cdatautils-text - INFO] Catch2 was not found. The test targets will not be created.")
endif()
