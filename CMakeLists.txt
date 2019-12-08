cmake_minimum_required (VERSION 3.1)

project(numbdb LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

include(CheckSymbolExists)
include(AddCompilerFlags)
include(FindReadline)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_library(Readline REQUIRED)

# Compiler Condition (gcc ; clang)
if ( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_C_COMPILER_ID}" STREQUAL "CLANG")
  message( STATUS "Setting the flags for ${CMAKE_C_COMPILER_ID} compiler" )
  add_definitions( --std=c99 )
endif()

# Check for OS
if( UNIX )
    set( LEARN_CMAKE_DEFINITIONS
        "${LEARN_CMAKE_DEFINITIONS} -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-parameter -Wno-comment" )
endif()
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_C_FLAGS "$ENV{CFLAGS} -O2 -Wall")
#if ("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "debug")
#  add_compiler_flags(-g -O0 -Wall -ggdb)
#endif()

# POLLER choice
set(POLLER "")

if(POLLER STREQUAL "")
    check_symbol_exists(epoll_create sys/epoll.h HAVE_EPOLL)
    if(HAVE_EPOLL)
        set(POLLER "epoll")
        message("Use epoll")
        add_definitions(-DHAVE_EPOLL)
    endif()
endif()

if(POLLER STREQUAL "")
    check_symbol_exists(kqueue sys/event.h HAVE_KQUEUE)
    if(HAVE_KQUEUE)
        set(POLLER "kqueue")
        message("Use kqueue")
        add_definitions(-DHAVE_KQUEUE)
    endif()
endif()

if(POLLER STREQUAL "")
    set(POLLER "select")
    message("Use select")
    add_definitions(-DHAVE_SELECT)
endif()

#aux_source_directory(. DIR_SRC)
#file(GLOB DIR_SRC "*.c" "*.h")
#list(REMOVE_ITEM DIR_SRC ${PROJECT_SOURCE_DIR}/ae_epoll.c)

set(NUMBDB_FILES
  numbdb.c
  ae.c
  anet.c
  dict.c
  sds.c
  zmalloc.c)

set(NUMBDB_CLI_FILES
  numbdb-cli.c
  sds.c
  anet.c
  zmalloc.c)


add_executable(${PROJECT_NAME} ${NUMBDB_FILES})
target_link_libraries(${PROJECT_NAME} Threads::Threads)

add_executable(numbdb-cli ${NUMBDB_CLI_FILES})
target_link_libraries(numbdb-cli ${READLINE_LIBRARIES})

add_subdirectory(tests)
