# - Try to find mysql
# Once done this will define
#
#  MYSQL_FOUND - system has mysql
#  MYSQL_INCLUDE_DIRS - the mysql include directory
#  MYSQL_LIBRARIES - Link these to use mysql
#  MYSQL_DEFINITIONS - Compiler switches required for using mysql
#
#  Copyright (c) 2010 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (MYSQL_LIBRARIES AND MYSQL_INCLUDE_DIRS)
  # in cache already
  set(MYSQL_FOUND TRUE)
else (MYSQL_LIBRARIES AND MYSQL_INCLUDE_DIRS)
  find_path(MYSQL_INCLUDE_DIR
    NAMES
      mysql/mysql.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /opt/local/include/mysql5
      /sw/include
  )

  find_library(MYSQLCLIENT_LIBRARY
    NAMES
      mysqlclient
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /opt/local/lib/mysql5/mysql
      /sw/lib
  )

  set(MYSQL_INCLUDE_DIRS
    ${MYSQL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR}/mysql
  )

  set(MYSQL_LIBRARIES
    ${MYSQLCLIENT_LIBRARY}
)

  if (MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARIES)
     set(MYSQL_FOUND TRUE)
  endif (MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARIES)

  if (MYSQL_FOUND)
    if (NOT mysql_FIND_QUIETLY)
      message(STATUS "Found mysql: ${MYSQL_LIBRARIES}")
    endif (NOT mysql_FIND_QUIETLY)
  else (MYSQL_FOUND)
    if (mysql_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find mysql")
    endif (mysql_FIND_REQUIRED)
  endif (MYSQL_FOUND)

  # show the MYSQL_INCLUDE_DIRS and MYSQL_LIBRARIES variables only in the advanced view
  mark_as_advanced(MYSQL_INCLUDE_DIRS MYSQL_LIBRARIES)

endif (MYSQL_LIBRARIES AND MYSQL_INCLUDE_DIRS)

