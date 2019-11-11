cmake_minimum_required(VERSION 3.14)

project(Morpha C)

include(CTest)
enable_testing()

add_library(morpha morpha.h morpha.c)
target_include_directories(morpha INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_executable(main_test main_test.c)
target_link_libraries(main_test morpha)

add_test(main main_test)