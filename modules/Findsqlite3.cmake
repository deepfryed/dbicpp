# - Try to find sqlit3
# Once done this will define
#
#  SQLITE3_FOUND - system has sqlite3
#  SQLITE3_INCLUDE_DIRS - the sqlite3 include directory
#  SQLITE3_LIBRARIES - Link these to use sqlite3
#  SQLITE3_DEFINITIONS - Compiler switches required for using sqlite3
#
#  Copyright (c) 2011 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (SQLITE3_LIBRARIES AND SQLITE3_INCLUDE_DIRS)
  # in cache already
  set(SQLITE3_FOUND TRUE)
else (SQLITE3_LIBRARIES AND SQLITE3_INCLUDE_DIRS)
  find_path(SQLITE3_INCLUDE_DIR
    NAMES
      sqlite3.h
    PATHS
      /usr/include
      /usr/include/sqlite3
      /usr/local/include
      /usr/local/include/sqlite3
      /opt/local/include
      /opt/local/include/sqlite3
      /sw/include
  )

  find_library(SQLITE3_LIBRARY
    NAMES
      sqlite3
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /opt/local/lib/sqlite3
      /sw/lib
  )

  set(SQLITE3_INCLUDE_DIRS
    ${SQLITE3_INCLUDE_DIR}
  )
  set(SQLITE3_LIBRARIES
    ${SQLITE3_LIBRARY}
)

  if (SQLITE3_INCLUDE_DIRS AND SQLITE3_LIBRARIES)
     set(SQLITE3_FOUND TRUE)
  endif (SQLITE3_INCLUDE_DIRS AND SQLITE3_LIBRARIES)

  if (SQLITE3_FOUND)
    if (NOT sqlite3_FIND_QUIETLY)
      message(STATUS "Found sqlite3: ${SQLITE3_LIBRARIES}")
    endif (NOT sqlite3_FIND_QUIETLY)
  else (SQLITE3_FOUND)
    if (sqlite3_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find sqlite3")
    endif (sqlite3_FIND_REQUIRED)
  endif (SQLITE3_FOUND)

  # show the SQLITE3_INCLUDE_DIRS and SQLITE3_LIBRARIES variables only in the advanced view
  mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)

endif (SQLITE3_LIBRARIES AND SQLITE3_INCLUDE_DIRS)

