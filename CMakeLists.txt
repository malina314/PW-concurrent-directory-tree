###############################################
# oryginalny cmake
# todo: dodać w nim ReadWriteLock.c i path_utils.c do `add_library(Tree Tree.c)`
#
#cmake_minimum_required(VERSION 3.8)
#project(MIMUW-FORK C)
#
#set(CMAKE_CXX_STANDARD "17")
#set(CMAKE_C_STANDARD "11")
#set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare")
#
#add_library(err err.c)
#add_library(HashMap HashMap.c)
#add_library(Tree Tree.c)
#add_executable(main main.c)
#target_link_libraries(main Tree HashMap err pthread)
#
#install(TARGETS DESTINATION .)
###############################################


cmake_minimum_required(VERSION 3.8)
project(MIMUW-FORK C)

set(CMAKE_C_STANDARD "11")

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare")

#
#add_library(err err.c)
#add_library(HashMap HashMap.c)
#add_library(path_utils path_utils.c)
#add_library(Tree path_utils.c ReadWriteLock.c Tree.c)
# biblioteki trzeba dodać 3 linijki niżej do target_link_libraries
##add_executable(main main.c)
#include("${CMAKE_CURRENT_SOURCE_DIR}/testy-zad2/CMakeExtension.txt")
#target_link_libraries(main Tree HashMap err pthread)
#
#target_link_libraries(main_dbg Tree HashMap err pthread)
#target_compile_options(main_dbg PRIVATE "-fsanitize=undefined")
#target_compile_options(main_dbg PRIVATE "-fsanitize=address")
#target_compile_options(main_dbg PRIVATE "-g")
#
#install(TARGETS DESTINATION .)

set(TESTS_PATH "testy-zad2/")

add_executable(bez_sanitizerow
        "${TESTS_PATH}test.c"
        "${TESTS_PATH}utils.c"
        "${TESTS_PATH}valid_path.c"
        "${TESTS_PATH}sequential_small.c"
        "${TESTS_PATH}sequential_big_random.c"
        "${TESTS_PATH}deadlock.c"
        "${TESTS_PATH}concurrent_same_as_some_sequential.c"
        "${TESTS_PATH}liveness.c"
        err.c
        HashMap.c
        path_utils.c
        ReadWriteLock.c
        Tree.c)

target_link_libraries(bez_sanitizerow pthread)


add_executable(z_sanitizerami
        "${TESTS_PATH}test.c"
        "${TESTS_PATH}utils.c"
        "${TESTS_PATH}valid_path.c"
        "${TESTS_PATH}sequential_small.c"
        "${TESTS_PATH}sequential_big_random.c"
        "${TESTS_PATH}deadlock.c"
        "${TESTS_PATH}concurrent_same_as_some_sequential.c"
        "${TESTS_PATH}liveness.c"
        err.c
        HashMap.c
        path_utils.c
        ReadWriteLock.c
        Tree.c)

target_compile_options(z_sanitizerami PRIVATE "-fsanitize=undefined")
target_compile_options(z_sanitizerami PRIVATE "-fsanitize=address")
target_compile_options(z_sanitizerami PRIVATE "-g")

target_link_libraries(z_sanitizerami pthread asan static-libasan)

