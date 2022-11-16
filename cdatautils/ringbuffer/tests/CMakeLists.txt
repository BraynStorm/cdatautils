if(Catch2_FOUND)
    include(CTest)
    include(Catch)

    add_executable(cdatautils-ringbuffer-test ringbuffer-test.cpp)

    target_link_libraries(cdatautils-ringbuffer-test PUBLIC ringbuffer Catch2::Catch2WithMain)

    set_target_properties(
        cdatautils-ringbuffer-test
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED 17
        CXX_EXTENSIONS OFF
    )

    catch_discover_tests(cdatautils-ringbuffer-test)
else()
    message("[cdatautils-ringbuffer - INFO] Catch2 was not found. The test targets will not be created.")
endif()
