# - Try to find pcre
# Once done this will define
#
#  PCRE_FOUND - system has pcre
#  PCRE_INCLUDE_DIRS - the pcre include directory
#  PCRE_LIBRARIES - Link these to use pcre
#  PCRE_DEFINITIONS - Compiler switches required for using pcre
#
#  Copyright (c) 2010 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (PCRE_LIBRARIES AND PCRE_INCLUDE_DIRS)
  # in cache already
  set(PCRE_FOUND TRUE)
else (PCRE_LIBRARIES AND PCRE_INCLUDE_DIRS)
  find_path(PCRE_INCLUDE_DIR
    NAMES
      pcrecpp.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(PCRECPP_LIBRARY
    NAMES
      pcrecpp
    PATHS
      /usr/lib
      /usr/lib/x86_64-linux-gnu
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(PCRE_INCLUDE_DIRS
    ${PCRE_INCLUDE_DIR}
  )
  set(PCRE_LIBRARIES
    ${PCRECPP_LIBRARY}
)

  if (PCRE_INCLUDE_DIRS AND PCRE_LIBRARIES)
     set(PCRE_FOUND TRUE)
  endif (PCRE_INCLUDE_DIRS AND PCRE_LIBRARIES)

  if (PCRE_FOUND)
    if (NOT pcre_FIND_QUIETLY)
      message(STATUS "Found pcre: ${PCRE_LIBRARIES}")
    endif (NOT pcre_FIND_QUIETLY)
  else (PCRE_FOUND)
    if (pcre_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find pcre")
    endif (pcre_FIND_REQUIRED)
  endif (PCRE_FOUND)

  # show the PCRE_INCLUDE_DIRS and PCRE_LIBRARIES variables only in the advanced view
  mark_as_advanced(PCRE_INCLUDE_DIRS PCRE_LIBRARIES)

endif (PCRE_LIBRARIES AND PCRE_INCLUDE_DIRS)

