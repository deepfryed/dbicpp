#include "common.h"

namespace dbi {

    char errormsg[8192];

    const char *typemap[] = {
        "%d", "int", "%u", "int", "%lu", "int", "%ld", "int",
        "%f", "float", "%lf", "float",
        "%s", "text"
    };

    void PQ_NOTICE(void *arg, const char *message) {
        if (_trace) logMessage(_trace_fd, message);
    }

    void PQ_PREPROCESS_QUERY(string &normalized_sql) {
        int i, n = 0;
        char repl[128];
        string var;
        RE re("(?<!')(%(?:[dsfu]|l[dfu])|\\?)(?!')");

        while (re.PartialMatch(normalized_sql, &var)) {
            for (i = 0; i < (int)(sizeof(typemap)/sizeof(char *)); i += 2) {
                if (var == typemap[i]) {
                    sprintf(repl, "$%d::%s", ++n, typemap[i+1]);
                    re.Replace(repl, &normalized_sql);
                    i = -1;
                    break;
                }
            }
            if (i != -1) {
                sprintf(repl, "$%d", ++n);
                re.Replace(repl, &normalized_sql);
            }
        }
    }

    void PQ_PROCESS_BIND(const char ***param_v, int **param_l, int **param_f, param_list_t &bind) {
        *param_v = new const char*[bind.size()];
        *param_l = new int[bind.size()];
        *param_f = new int[bind.size()];

        for (uint32_t i = 0; i < bind.size(); i++) {
            bool isnull = bind[i].isnull;
            (*param_v)[i] = isnull ? 0 : bind[i].value.data();
            (*param_l)[i] = isnull ? 0 : bind[i].value.length();
            (*param_f)[i] = bind[i].binary ? 1 : 0;
        }
    }

    void PQ_CHECK_RESULT(PGresult *result, PGconn *conn, string sql) {
        bool cerror;
        switch(PQresultStatus(result)) {
            case PGRES_TUPLES_OK:
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_EMPTY_QUERY:
            case PGRES_COMMAND_OK:
                break;
            case PGRES_BAD_RESPONSE:
            case PGRES_FATAL_ERROR:
            case PGRES_NONFATAL_ERROR:
                cerror = PQstatus(conn) == CONNECTION_BAD;
                snprintf(errormsg, 8192, "In SQL: %s\n\n %s", sql.c_str(), PQresultErrorMessage(result));
                PQclear(result);
                if (cerror)
                    throw ConnectionError((const char*)errormsg);
                else
                    throw RuntimeError((const char*)errormsg);
                break;
            default:
                cerror = PQstatus(conn) == CONNECTION_BAD;
                snprintf(errormsg, 8192, "In SQL: %s\n\n Unknown error, check logs.", sql.c_str());
                PQclear(result);
                if (cerror)
                    throw ConnectionError((const char*)errormsg);
                else
                    throw RuntimeError((const char*)errormsg);
                break;
        }
    }
}

using namespace std;
using namespace dbi;

extern "C" {
    PgHandle* dbdConnect(string user, string pass, string dbname, string host, string port, char *options) {
        if (host == "")
            host = "127.0.0.1";
        if (port == "0" || port == "")
            port = "5432";

        return new PgHandle(user, pass, dbname, host, port, options);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
