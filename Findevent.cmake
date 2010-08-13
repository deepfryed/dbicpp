# - Try to find event
# Once done this will define
#
#  EVENT_FOUND - system has event
#  EVENT_INCLUDE_DIRS - the event include directory
#  EVENT_LIBRARIES - Link these to use event
#  EVENT_DEFINITIONS - Compiler switches required for using event
#
#  Copyright (c) 2010 Bharanee Rathna <deepfryed@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (EVENT_LIBRARIES AND EVENT_INCLUDE_DIRS)
  # in cache already
  set(EVENT_FOUND TRUE)
else (EVENT_LIBRARIES AND EVENT_INCLUDE_DIRS)
  find_path(EVENT_INCLUDE_DIR
    NAMES
      event.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(EVENT_LIBRARY
    NAMES
      event
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(EVENT_INCLUDE_DIRS
    ${EVENT_INCLUDE_DIR}
  )
  set(EVENT_LIBRARIES
    ${EVENT_LIBRARY}
)

  if (EVENT_INCLUDE_DIRS AND EVENT_LIBRARIES)
     set(EVENT_FOUND TRUE)
  endif (EVENT_INCLUDE_DIRS AND EVENT_LIBRARIES)

  if (EVENT_FOUND)
    if (NOT event_FIND_QUIETLY)
      message(STATUS "Found event: ${EVENT_LIBRARIES}")
    endif (NOT event_FIND_QUIETLY)
  else (EVENT_FOUND)
    if (event_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find event")
    endif (event_FIND_REQUIRED)
  endif (EVENT_FOUND)

  # show the EVENT_INCLUDE_DIRS and EVENT_LIBRARIES variables only in the advanced view
  mark_as_advanced(EVENT_INCLUDE_DIRS EVENT_LIBRARIES)

endif (EVENT_LIBRARIES AND EVENT_INCLUDE_DIRS)

