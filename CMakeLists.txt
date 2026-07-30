cmake_minimum_required(VERSION 3.16)
project(embedded_serial_link CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(ESL_BUILD_APP "Собирать демо-приложение" ON)
option(ESL_BUILD_TESTS "Собирать тесты на doctest" ON)
option(ESL_STRICT_EMBEDDED
       "Собирать esl_core без исключений/RTTI, как на реальной прошивке MCU" OFF)

add_subdirectory(src)

if(ESL_BUILD_APP)
    add_subdirectory(app)
endif()

if(ESL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(third_party)
    add_subdirectory(tests)
endif()
