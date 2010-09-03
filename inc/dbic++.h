#ifndef _DBICXX_BASE_H
#define _DBICXX_BASE_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <map>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pcrecpp.h>
#include <uuid/uuid.h>
#include <stdint.h>

#define DBI_VERSION      0.3.1

namespace dbi {
    struct null {};
    struct execute {};
}

#include "dbic++/error.h"
#include "dbic++/param.h"
#include "dbic++/container.h"
#include "dbic++/cpool.h"

#define DEFAULT_DRIVER_PATH "/usr/lib/dbic++"

#define DBI_TYPE_INT     1
#define DBI_TYPE_TIME    2
#define DBI_TYPE_TEXT    3
#define DBI_TYPE_FLOAT   4
#define DBI_TYPE_NUMERIC 5
#define DBI_TYPE_BOOLEAN 6
#define DBI_TYPE_BLOB    7
#define DBI_TYPE_DATE    8

namespace dbi {
    extern bool _trace;
    extern int  _trace_fd;

    class AbstractStatement;
    class AbstractResult;
}

#include "dbic++/util.h"
#include "dbic++/io.h"
#include "dbic++/io_stream.h"
#include "dbic++/io_filestream.h"
#include "dbic++/abstract_handle.h"
#include "dbic++/abstract_result.h"
#include "dbic++/abstract_statement.h"
#include "dbic++/handle.h"
#include "dbic++/statement.h"

#endif
