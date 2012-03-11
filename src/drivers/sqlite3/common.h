#ifndef _DBICXX_SQLITE3_COMMON_H
#define _DBICXX_SQLITE3_COMMON_H

#include "dbic++.h"
#include <sqlite3.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DRIVER_NAME     "sqlite3"
#define DRIVER_VERSION  "1.3"

namespace dbi {
    extern char errormsg[8192];
    void SQLITE3_PREPROCESS_QUERY(string &query);
    void SQLITE3_PROCESS_BIND(sqlite3_stmt *, param_list_t &bind);
}

#include "result.h"
#include "statement.h"
#include "handle.h"

#endif
