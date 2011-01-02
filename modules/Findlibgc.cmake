# - Try to find libgc
# Once done this will define
#
#  LIBGC_FOUND - system has libgc
#  LIBGC_INCLUDE_DIRS - the libgc include directory
#  LIBGC_LIBRARIES - Link these to use libgc
#  LIBGC_DEFINITIONS - Compiler switches required for using libgc
#
#  Copyright (c) 2010 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBGC_LIBRARIES AND LIBGC_INCLUDE_DIRS)
  # in cache already
  set(LIBGC_FOUND TRUE)
else (LIBGC_LIBRARIES AND LIBGC_INCLUDE_DIRS)
  find_path(LIBGC_INCLUDE_DIR
    NAMES
      gc/gc.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(LIBGC_LIBRARY
    NAMES
      gc
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(LIBGC_INCLUDE_DIRS
    ${LIBGC_INCLUDE_DIR}
  )
  set(LIBGC_LIBRARIES
    ${LIBGC_LIBRARY}
)

  if (LIBGC_INCLUDE_DIRS AND LIBGC_LIBRARIES)
     set(LIBGC_FOUND TRUE)
  endif (LIBGC_INCLUDE_DIRS AND LIBGC_LIBRARIES)

  if (LIBGC_FOUND)
    if (NOT libgc_FIND_QUIETLY)
      message(STATUS "Found libgc: ${LIBGC_LIBRARIES}")
    endif (NOT libgc_FIND_QUIETLY)
  else (LIBGC_FOUND)
    if (libgc_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libgc")
    endif (libgc_FIND_REQUIRED)
  endif (LIBGC_FOUND)

  # show the LIBGC_INCLUDE_DIRS and LIBGC_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBGC_INCLUDE_DIRS LIBGC_LIBRARIES)

endif (LIBGC_LIBRARIES AND LIBGC_INCLUDE_DIRS)

