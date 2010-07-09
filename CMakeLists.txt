cmake_minimum_required(VERSION 2.8)
PROJECT(DBICPP CXX C)
INCLUDE_DIRECTORIES(inc /usr/include/postgresql /usr/include/mysql)

LINK_DIRECTORIES(lib)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

ADD_LIBRARY(dbic++ STATIC src/dbic++.cc src/container.cc src/param.cc src/error.cc)

ADD_LIBRARY(dbdpg SHARED src/drivers/pg.cc)
TARGET_LINK_LIBRARIES(dbdpg pq)
SET_TARGET_PROPERTIES(dbdpg PROPERTIES VERSION ${CMAKE_PG_VERSION} SOVERSION 1)

ADD_LIBRARY(dbdmysql SHARED src/drivers/mysql.cc)
TARGET_LINK_LIBRARIES(dbdmysql mysqlclient)
SET_TARGET_PROPERTIES(dbdmysql PROPERTIES VERSION ${CMAKE_MYSQL_VERSION} SOVERSION 1)

ADD_EXECUTABLE(example src/example.cc)
TARGET_LINK_LIBRARIES(example dbic++ uuid pcrecpp dl)

SET(CMAKE_CXX_FLAGS "-Wunused-function -Wunused-value -Wunused-label -rdynamic -fPIC")

# Use -DCMAKE_INSTALL_PREFIX:PATH=<path> to override /usr/local
# SET(CMAKE_INSTALL_PREFIX /usr CACHE INTERNAL "")

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

INSTALL(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/dbic++)

INSTALL(TARGETS dbic++
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL(TARGETS dbdpg dbdmysql
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/dbic++
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/dbic++)

INSTALL(DIRECTORY inc/
        DESTINATION include
        FILES_MATCHING
        PATTERN "*")
