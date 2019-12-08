find_path(Readline_ROOT_DIR
    NAMES include/readline/readline.h
    PATHS /usr/local/opt/readline/ /opt/local/ /usr/local/ /usr/
    NO_DEFAULT_PATH
)
find_path(Readline_INCLUDE_DIR
    NAMES readline/readline.h
    PATHS ${Readline_ROOT_DIR}/include
    NO_DEFAULT_PATH
)

find_library(Readline_LIBRARY
    NAMES readline
    PATHS ${Readline_ROOT_DIR}/lib
    NO_DEFAULT_PATH
)
if (NOT Readline_LIBRARY)
 message(FATAL_ERROR "Not found libreadline.a")
endif ()
find_library(History_LIBRARY
    NAMES history
    PATHS ${Readline_ROOT_DIR}/lib
    NO_DEFAULT_PATH
)
if (NOT History_LIBRARY)
 message(FATAL_ERROR "Not found libhistory.a")
endif ()

if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  set(READLINE_FOUND TRUE)
else(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  FIND_LIBRARY(Readline_LIBRARY NAMES readline PATHS Readline_ROOT_DIR)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY )
  MARK_AS_ADVANCED(Readline_INCLUDE_DIR Readline_LIBRARY)
endif(Readline_INCLUDE_DIR AND Readline_LIBRARY)

mark_as_advanced(
    Readline_ROOT_DIR
    Readline_INCLUDE_DIR
    Readline_LIBRARY
)

set(READLINE_INCLUDE_DIR ${Readline_INCLUDE_DIR})
set(READLINE_LIBRARY ${Readline_LIBRARY} ${History_LIBRARY})

find_package(Curses REQUIRED)

if (READLINE_FOUND)
 set(READLINE_LIBRARIES ${READLINE_LIBRARY})
 list(APPEND READLINE_LIBRARIES ${CURSES_LIBRARIES})
 set(READLINE_INCLUDE_DIRS ${READLINE_INCLUDE_DIR} ${CURSES_INCLUDE_DIR})
endif ()
