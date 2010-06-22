#include "dbic++.h"
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <uuid/uuid.h>

#define DRIVER_NAME     "postgresql"
#define DRIVER_VERSION  "1.0.1"


#define PQTUPLES(r) (int __n = PQntuples(r); n

namespace dbi {

    const char *typemap[] = {
        "%d", "int", "%u", "int", "%lu", "int", "%ld", "int",
        "%f", "float", "%lf", "float",
        "%s", "text"
    };

    inline int PQNTUPLES(PGresult *r) {
        int n = PQntuples(r);
        return n > 0 ? n : atoi(PQcmdTuples(r));
    }

    void pgCheckResult(PGresult *result, string sql) {
        switch(PQresultStatus(result)) {
            case PGRES_TUPLES_OK:
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_EMPTY_QUERY:
            case PGRES_COMMAND_OK:
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
            bool isnull = bind[i].isnull;
            (*param_v)[i] = isnull ? 0 : bind[i].value.data();
            (*param_l)[i] = isnull ? 0 : bind[i].value.length();
        }
    }

    class PgStatement : public AbstractStatement {
        private:
        string sql;
        string _uuid;
        PGresult *_result;
        PGconn *conn;
        vector<string> _result_fields;
        unsigned int _rowno, _rows, _cols;

        void init() {
            _result = 0;
            _rowno  = 0;
            _rows   = 0;
            _cols   = 0;
            _uuid   = generateCompactUUID();
        }

        public:
        PgStatement() {}
        ~PgStatement() { finish(); }
        PgStatement(string query,  PGconn *c) {
            init();
            conn = c;
            sql = query;
            pgPreProcessQuery(query);
            PGresult *result = PQprepare(conn, _uuid.c_str(), query.c_str(), 0, 0);
            if (!result) throw RuntimeError("Unable to allocate statement");
            pgCheckResult(result, sql);
            PQclear(result);
        }

        string command() {
            return sql;
        }

        void check_ready(string m) {
            if (!_result)
                throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
        }

        unsigned int rows() {
            check_ready("rows()");
            return _rows;
        }

        unsigned int execute() {
            _result_fields.clear();
            _result = PQexecPrepared(conn, _uuid.c_str(), 0, 0, 0, 0, 0);
            pgCheckResult(_result, sql);
            _rows = (unsigned int)PQNTUPLES(_result);
            _cols = (unsigned int)PQnfields(_result);
            return _rows;
        }

        unsigned int execute(vector<Param> &bind) {
            int *param_l;
            const char **param_v;
            _result_fields.clear();
            pgProcessBindParams(&param_v, &param_l, bind);
            _result = PQexecPrepared(conn, _uuid.c_str(), bind.size(),
                                       (const char* const *)param_v, (const int*)param_l, 0, 0);
            delete []param_v;
            delete []param_l;
            pgCheckResult(_result, sql);
            bind.clear();
            _rows = (unsigned int)PQNTUPLES(_result);
            _cols = (unsigned int)PQnfields(_result);
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
                        PQgetisnull(_result, _rowno-1, i) ?
                              PARAM(null()) : PARAM(PQgetvalue(_result, _rowno-1, i))
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
                if (_result_fields.size() == 0) fields();
                for (unsigned int i = 0; i < _cols; i++)
                    rs[_result_fields[i]] =
                        PQgetisnull(_result, _rowno-1, i) ? PARAM(null()) : PARAM(PQgetvalue(_result, _rowno-1, i));
            }
            return rs;
        }

        vector<string> fields() {
            check_ready("fields()");
            for (unsigned int i = 0; i < _cols; i++) _result_fields.push_back(PQfname(_result, i));
            return _result_fields;
        }

        unsigned int columns() {
            return _cols;
        }

        bool finish() {
            _rowno = _rows = _cols = 0;
            if (_result) PQclear(_result);
            _result = 0;
            return true;
        }

        unsigned char* fetchValue(int r, int c) {
            check_ready("fetchValue()");
            return PQgetisnull(_result, r, c) ? 0 : (unsigned char*)PQgetvalue(_result, r, c);
        }

        unsigned int currentRow() { return _rowno; }
        void advanceRow() { _rowno = _rowno <= _rows ? _rowno + 1 : _rowno; }
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
            unsigned int rows;
            PGresult *result = PQexec(conn, sql.c_str());
            pgCheckResult(result, sql);
            rows = (unsigned int)PQNTUPLES(result);
            PQclear(result);
            return rows;
        }

        unsigned int execute(string sql, vector<Param> &bind) {
            int *param_l;
            const char **param_v;
            unsigned int rows;
            pgProcessBindParams(&param_v, &param_l, bind);
            PGresult *result = PQexecParams(conn, sql.c_str(), bind.size(),
                                            0, (const char* const *)param_v, param_l, 0, 0);
            delete []param_v;
            delete []param_l;
            pgCheckResult(result, sql);
            rows = (unsigned int)PQNTUPLES(result);
            PQclear(result);
            return rows;
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
        if (host == "") host = "127.0.0.1";
        if (port == "0" || port == "") port = "5432";
        return new PgHandle(user, pass, dbname, host, port);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
