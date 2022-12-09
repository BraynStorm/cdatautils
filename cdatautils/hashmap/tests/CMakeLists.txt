if(Catch2_FOUND)
    include(CTest)
    include(Catch)

    add_executable(cdatautils-hashmap-test hashmap-test.cpp)

    target_link_libraries(cdatautils-hashmap-test PUBLIC hashmap Catch2::Catch2WithMain)

    set_target_properties(
        cdatautils-hashmap-test
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED 17
        CXX_EXTENSIONS OFF
    )

    catch_discover_tests(cdatautils-hashmap-test)
else()
    message("[cdatautils-hashmap - INFO] Catch2 was not found. The test targets will not be created.")
endif()
