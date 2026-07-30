add_library(esl_core STATIC
    util/Logger.cpp
    serial/InMemoryLoopbackDriver.cpp
    serial/Rs485StubDriver.cpp
    serial/UsbCdcStubDriver.cpp
    protocol/Frame.cpp
    protocol/SerialLinkProtocol.cpp
)

target_include_directories(esl_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(esl_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(ESL_STRICT_EMBEDDED)
    target_compile_options(esl_core PUBLIC
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-exceptions -fno-rtti>
    )
endif()
