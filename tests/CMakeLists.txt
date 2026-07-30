add_executable(esl_tests
    doctest_main.cpp
    test_ring_buffer.cpp
    test_frame.cpp
    test_drivers.cpp
    test_protocol_integration.cpp
)

target_link_libraries(esl_tests PRIVATE esl_core doctest::doctest)

add_test(NAME esl_tests COMMAND esl_tests)
