# doctest подтягивается в third_party/doctest через FetchContent, а не
# системным пакетом. Для офлайн-сборки положите заголовок вручную по пути
# third_party/doctest/doctest/doctest.h (как в оригинальном репозитории) —
# если каталог уже заполнен, FetchContent_Declare ничего не скачивает.

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doctest
)

# Только populate, без add_subdirectory: cmake_minimum_required в
# CMakeLists.txt самого doctest слишком старый для актуальных версий CMake,
# а нам всё равно нужен только заголовочный файл.
FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
    FetchContent_Populate(doctest)
endif()

add_library(doctest INTERFACE)
add_library(doctest::doctest ALIAS doctest)
target_include_directories(doctest INTERFACE ${doctest_SOURCE_DIR})
