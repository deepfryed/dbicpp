# - Try to find db2
# Once done this will define
#
#  DB2_FOUND - system has db2
#  DB2_INCLUDE_DIRS - the db2 include directory
#  DB2_LIBRARIES - Link these to use db2
#  DB2_DEFINITIONS - Compiler switches required for using db2
#
#  Copyright (c) 2010 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (DB2_LIBRARIES AND DB2_INCLUDE_DIRS)
  # in cache already
  set(DB2_FOUND TRUE)
else (DB2_LIBRARIES AND DB2_INCLUDE_DIRS)
  find_path(DB2_INCLUDE_DIR
    NAMES
      sqlcli1.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /opt/ibm/db2/V9.*/include
      /sw/include
  )

  find_library(DB2CLIENT_LIBRARY
    NAMES
      db2
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /opt/ibm/db2/V9.*/lib
      /sw/lib
  )

  set(DB2_INCLUDE_DIRS
    ${DB2_INCLUDE_DIR}
  )
  set(DB2_LIBRARIES
    ${DB2CLIENT_LIBRARY}
)

  if (DB2_INCLUDE_DIRS AND DB2_LIBRARIES)
     set(DB2_FOUND TRUE)
  endif (DB2_INCLUDE_DIRS AND DB2_LIBRARIES)

  if (DB2_FOUND)
    if (NOT db2_FIND_QUIETLY)
      message(STATUS "Found db2: ${DB2_LIBRARIES}")
    endif (NOT db2_FIND_QUIETLY)
  else (DB2_FOUND)
    if (db2_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find db2")
    endif (db2_FIND_REQUIRED)
  endif (DB2_FOUND)

  # show the DB2_INCLUDE_DIRS and DB2_LIBRARIES variables only in the advanced view
  mark_as_advanced(DB2_INCLUDE_DIRS DB2_LIBRARIES)

endif (DB2_LIBRARIES AND DB2_INCLUDE_DIRS)

