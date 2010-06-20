#include "dbic++.h"
#include <internal/libpq-int.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <uuid/uuid.h>

#define DRIVER_NAME     "postgresql"
#define DRIVER_VERSION  "1.0.1"

namespace dbi {

    const char *typemap[] = {
        "d", "int", "u", "int", "lu", "int", "ld", "int",
        "f", "float", "lf", "float",
        "s", "text"
    };

    void pgCheckResult(PGresult *result, string sql) {
        char *cstatus;
        switch(PQresultStatus(result)) {
            case PGRES_TUPLES_OK:
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_EMPTY_QUERY:
                return;
            case PGRES_COMMAND_OK:
                cstatus = PQcmdTuples(result);
                if (strcmp(cstatus, "") != 0) result->ntups = atoi(cstatus);
                return;
            case PGRES_BAD_RESPONSE:
            case PGRES_FATAL_ERROR:
            case PGRES_NONFATAL_ERROR:
                throw RuntimeError(PQresultErrorMessage(result) + string(" - in query: ") + sql);
                break;
            default:
                throw RuntimeError("Unknown error, check logs." + string(" - in query: ") + sql);
        }
    }

    string generateCompactUUID() {
        string rv;
        char hex[3];
        unsigned char uuid[16];
        uuid_generate(uuid);
        for (int i = 0; i < 16; i++) {
            sprintf(hex, "%02X", uuid[i]);
            rv += hex;
        }
        return rv;
    }

    // TODO: raise parser errors here before sending it to server ?
    void pgPreProcessQuery(string &query) {
        int i, n = 0;
        char repl[128];
        string var;
        RE re("(?<!')(%(?:[dsfu]|l[dfu])|\\?)(?!')");
        while (re.PartialMatch(query, &var)) {
            for (i = 0; i < (int)(sizeof(typemap)/sizeof(char *)); i += 2) {
                if (var == typemap[i]) {
                    sprintf(repl, "$%d::%s", ++n, typemap[i+1]);
                    re.Replace(repl, &query);
                    i = -1;
                    break;
                }
            }
            if (i != -1) {
                sprintf(repl, "$%d", ++n);
                re.Replace(repl, &query);
            }
        }
    }

    void pgProcessBindParams(const char ***param_v, int **param_l, vector<Param> &bind) {
        *param_v = new const char*[bind.size()];
        *param_l = new int[bind.size()];
        for (unsigned int i = 0; i < bind.size(); i++) {
            bool isnull = bind[i].isnull();
            (*param_v)[i] = isnull ? 0 : bind[i].str().data();
            (*param_l)[i] = isnull ? 0 : bind[i].str().length();
            // TODO figure out where the Schrodinger cat is hiding. The following line
            // prevents some sort of eager compiler optimization of str().data() values.
            bind[i].str().data();
        }
    }

    class PgStatement : public AbstractStatement {
        private:
        string sql;
        string st_uuid;
        PGresult *st_result;
        PGconn *conn;
        vector<string> st_result_columns;
        unsigned int _rowno, _rows, _cols;

        void init() {
            st_result = 0;
            st_uuid   = generateCompactUUID();
            _rowno    = 0;
            _rows     = 0;
            _cols     = 0;
        }

        public:
        PgStatement() {}
        ~PgStatement() { finish(); }
        PgStatement(string query,  PGconn *c) {
            init();
            conn = c;
            sql = query;
            pgPreProcessQuery(query);
            PGresult *result = PQprepare(conn, st_uuid.c_str(), query.c_str(), 0, 0);
            if (!result) throw RuntimeError("Unable to allocate statement");
            pgCheckResult(result, sql);
            PQclear(result);
        }

        void check_ready(string m) {
            if (!st_result)
                throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
        }

        unsigned int rows() {
            check_ready("rows()");
            return _rows;
        }

        unsigned int execute() {
            st_result_columns.clear();
            st_result = PQexecPrepared(conn, st_uuid.c_str(), 0, 0, 0, 0, 0);
            pgCheckResult(st_result, sql);
            _rows = (unsigned int)PQntuples(st_result);
            _cols = (unsigned int)PQnfields(st_result);
            return _rows;
        }

        unsigned int execute(vector<Param> &bind) {
            int *param_l;
            const char **param_v;
            st_result_columns.clear();
            pgProcessBindParams(&param_v, &param_l, bind);
            st_result = PQexecPrepared(conn, st_uuid.c_str(), bind.size(),
                                       (const char* const *)param_v, (const int*)param_l, 0, 0);
            delete []param_v;
            delete []param_l;
            pgCheckResult(st_result, sql);
            bind.clear();
            _rows = (unsigned int)PQntuples(st_result);
            _cols = (unsigned int)PQnfields(st_result);
            return _rows;
        }

        ResultRow fetchRow() {
            check_ready("fetchRow()");
            ResultRow rs;
            rs.reserve(_cols);
            if (_rowno < _rows) {
                _rowno++;
                for (unsigned int i = 0; i < _cols; i++) {
                    rs.push_back(
                        PQgetisnull(st_result, _rowno-1, i) ?
                              Param(null()) : PQgetvalue(st_result, _rowno-1, i)
                    );
                }
            }
            return rs;
        }

        ResultRowHash fetchRowHash() {
            check_ready("fetchRowHash()");
            ResultRowHash rs;
            if (_rowno < _rows) {
                _rowno++;
                if (st_result_columns.size() == 0)
                    for (unsigned int i = 0; i < _cols; i++)
                        st_result_columns.push_back(PQfname(st_result, i));
                for (unsigned int i = 0; i < _cols; i++)
                    rs[st_result_columns[i]] = (
                        PQgetisnull(st_result, _rowno-1, i) ? Param(null()) : PQgetvalue(st_result, _rowno-1, i)
                    );
            }
            return rs;
        }

        bool finish() {
            _rowno = _rows = _cols = 0;
            if (st_result) PQclear(st_result);
            st_result = 0;
            return true;
        }

        unsigned char* fetchValue(int r, int c) {
            check_ready("fetchValue()");
            return (unsigned char*)PQgetvalue(st_result, r, c);
        }
    };

    class PgHandle : public AbstractHandle {
        private:
        PGconn *conn;
        int tr_nesting;
        public:
        PgHandle() { tr_nesting = 0; }
        PgHandle(string user, string pass, string dbname, string h, string p) {
            tr_nesting = 0;
            string conninfo;
            string host = h;
            string port = p;
            conninfo += "host='" + host + "' ";
            conninfo += "port='" + port + "' ";
            conninfo += "user='" + user + "' ";
            conninfo += "password='" + pass + "' ";
            conninfo += "dbname='" + dbname + "' ";
            conn = PQconnectdb(conninfo.c_str());
            if (!conn)
                throw ConnectionError("Unable to allocate db handle");
            else if (PQstatus(conn) == CONNECTION_BAD)
                throw ConnectionError(PQerrorMessage(conn));
        }

        ~PgHandle() {
            if (conn) PQfinish(conn);
        }

        unsigned int execute(string sql) {
            PGresult *result = PQexec(conn, sql.c_str());
            pgCheckResult(result, sql);
            unsigned int ntups = (unsigned int)result->ntups;
            PQclear(result);
            return ntups;
        }

        unsigned int execute(string sql, vector<Param> &bind) {
            int *param_l;
            const char **param_v;
            pgProcessBindParams(&param_v, &param_l, bind);
            PGresult *result = PQexecParams(conn, sql.c_str(), bind.size(),
                                            0, (const char* const *)param_v, param_l, 0, 0);
            delete []param_v;
            delete []param_l;
            pgCheckResult(result, sql);
            unsigned int ntups = (unsigned int)result->ntups;
            PQclear(result);
            return ntups;
        }

        PgStatement* prepare(string sql) {
            return new PgStatement(sql, this->conn);
        }

        bool begin() {
            execute("BEGIN WORK");
            tr_nesting++;
            return true;
        };

        bool commit() {
            execute("COMMIT");
            tr_nesting--;
            return true;
        };

        bool rollback() {
            execute("ROLLBACK");
            tr_nesting--;
            return true;
        };

        bool begin(string name) {
            if (tr_nesting == 0) begin();
            execute("SAVEPOINT " + name);
            tr_nesting++;
            return true;
        };

        bool commit(string name) {
            execute("RELEASE SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1) commit();
            return true;
        };

        bool rollback(string name) {
            execute("ROLLBACK TO SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1) rollback();
            return true;
        };

        void* call(string name, void* args) {
            return NULL;
        }

        bool close() {
            if (conn) PQfinish(conn);
            return true;
        }
    };
}

using namespace std;
using namespace dbi;

extern "C" {
    PgHandle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        if (port == "0") port = "5432";
        return new PgHandle(user, pass, dbname, host, port);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
