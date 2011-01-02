#ifndef _DBICXX_PG_COMMON_H
#define _DBICXX_PG_COMMON_H

#include "dbic++.h"
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DRIVER_NAME     "postgresql"
#define DRIVER_VERSION  "1.3"

#define PG2PARAM(res, r, c) PARAM((unsigned char*)PQgetvalue(res, r, c), PQgetlength(res, r, c))

namespace dbi {
    extern char errormsg[8192];
    extern const char *typemap[];
    void PQ_NOTICE(void *arg, const char *message);
    void PQ_PREPROCESS_QUERY(string &query);
    void PQ_PROCESS_BIND(const char ***param_v, int **param_l, int **param_f, vector<Param> &bind);
    void PQ_CHECK_RESULT(PGresult *result, string sql);
}

#include "result.h"
#include "statement.h"
#include "handle.h"

#endif
