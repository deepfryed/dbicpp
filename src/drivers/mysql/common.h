#ifndef _DBICXX_MYSQL_COMMON_H
#define _DBICXX_MYSQL_COMMON_H

#include "dbic++.h"
#include <cstdio>
#include <mysql/mysql.h>
#include <mysql/mysql_com.h>
#include <mysql/errmsg.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define DRIVER_NAME             "mysql"
#define DRIVER_VERSION          "1.3"
#define __MYSQL_BIND_BUFFER_LEN 2048

#define THROW_MYSQL_STMT_ERROR(s) {\
    snprintf(errormsg, 8192, "In SQL: %s\n\n %s", _sql.c_str(), mysql_stmt_error(s));\
    boom(errormsg);\
}

namespace dbi {

    extern char errormsg[8192];

    extern char MYSQL_BOOL_TRUE;
    extern char MYSQL_BOOL_FALSE;
    extern bool MYSQL_BIND_RO;
    extern bool MYSQL_BIND_RW;

    extern map<string, IO*> CopyInList;

    void MYSQL_PREPROCESS_QUERY(string &query);
    void MYSQL_INTERPOLATE_BIND(MYSQL *conn, string &query, vector<Param> &bind);
    bool MYSQL_CONNECTION_ERROR(int error);

    int  LOCAL_INFILE_INIT(void **ptr, const char *filename, void *unused);
    int  LOCAL_INFILE_READ(void *ptr, char *buffer, uint32_t len);
    void LOCAL_INFILE_END(void *ptr);
    int  LOCAL_INFILE_ERROR(void *ptr, char *error, uint32_t len);
}


#include "result.h"
#include "binary_result.h"
#include "statement.h"
#include "handle.h"

#endif
