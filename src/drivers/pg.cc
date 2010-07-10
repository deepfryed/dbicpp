#include "dbic++.h"
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <unistd.h>

#define DRIVER_NAME     "postgresql"
#define DRIVER_VERSION  "1.1"

#define PG2PARAM(res, r, c) PARAM_BINARY((unsigned char*)PQgetvalue(res, r, c), PQgetlength(res, r, c))

namespace dbi {

    char errormsg[8192];

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
                snprintf(errormsg, 8192, "In SQL: %s\n\n %s", sql.c_str(), PQresultErrorMessage(result));
                PQclear(result);
                throw RuntimeError((const char*)errormsg);
                break;
            default:
                snprintf(errormsg, 8192, "In SQL: %s\n\n Unknown error, check logs.", sql.c_str());
                PQclear(result);
                throw RuntimeError(errormsg);
                break;
        }
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
        protected:
        PGconn *conn;
        private:
        string _sql;
        string _uuid;
        PGresult *_result;
        vector<string> _rsfields;
        vector<int> _rsfield_types;
        unsigned int _rowno, _rows, _cols;
        ResultRow _rsrow;
        ResultRowHash _rsrowhash;
        bool _async;

        void init() {
            _result = 0;
            _rowno  = 0;
            _rows   = 0;
            _cols   = 0;
            _uuid   = generateCompactUUID();
            _async  = false;
        }

        public:
        PgStatement() {}
        ~PgStatement() { cleanup(); }
        PgStatement(string query,  PGconn *c, bool async = false) {
            int i;

            init();
            conn = c;
            _sql = query;

            pgPreProcessQuery(query);
            PGresult *result;

            // prepare is always sync - only execute is really async.
            if (async) {
                _async = true;
                result = asyncPrepare();
            }
            else {
                result = PQprepare(conn, _uuid.c_str(), query.c_str(), 0, 0);

                if (!result) throw RuntimeError("Unable to allocate statement");
                pgCheckResult(result, _sql);
                PQclear(result);

                result = PQdescribePrepared(conn, _uuid.c_str());
            }

            _cols  = (unsigned int)PQnfields(result);

            for (i = 0; i < (int)_cols; i++)
                _rsfields.push_back(PQfname(result, i));

            for (i = 0; i < (int)_cols; i++)
                _rsfield_types.push_back(PQfformat(result, i));

            _rsrow.reserve(_cols);
            PQclear(result);
        }

        PGresult* asyncPrepare() {
            int rc;
            PGresult *response, *result;
            string query = _sql;
            pgPreProcessQuery(query);

            rc = PQsendPrepare(conn, _uuid.c_str(), query.c_str(), 0, 0);
            if (rc == 0) throw RuntimeError(PQerrorMessage(conn));
            while (PQisBusy(conn)) PQconsumeInput(conn);
            result = PQgetResult(conn);
            while ((response = PQgetResult(conn))) PQclear(response);

            pgCheckResult(result, _sql);
            PQclear(result);

            rc = PQsendDescribePrepared(conn, _uuid.c_str());
            if (rc == 0) throw RuntimeError(PQerrorMessage(conn));
            while (PQisBusy(conn)) PQconsumeInput(conn);
            result = PQgetResult(conn);
            while ((response = PQgetResult(conn))) PQclear(response);

            return result;
        }

        void cleanup() {
            finish();
        }

        string command() {
            return _sql;
        }

        void checkReady(string m) {
            if (!_result)
                throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
        }

        unsigned int rows() {
            checkReady("rows()");
            return _rows;
        }

        unsigned int execute() {
            int rc;
            finish();

            if (_async) {
                rc = PQsendQueryPrepared(conn, _uuid.c_str(), 0, 0, 0, 0, 0);
                if (rc == 0) throw RuntimeError(PQerrorMessage(conn));
            }
            else {
                _result = PQexecPrepared(conn, _uuid.c_str(), 0, 0, 0, 0, 0);
                pgCheckResult(_result, _sql);
                _rows = (unsigned int)PQNTUPLES(_result);
            }

            // will return 0 for async queries.
            return _rows;
        }

        unsigned int execute(vector<Param> &bind) {
            int *param_l, rc;
            const char **param_v;

            finish();
            pgProcessBindParams(&param_v, &param_l, bind);

            if (_async) {
                rc = PQsendQueryPrepared(conn, _uuid.c_str(), bind.size(),
                                           (const char* const *)param_v, (const int*)param_l, 0, 0);
                delete []param_v;
                delete []param_l;

                if (rc == 0) throw RuntimeError(PQerrorMessage(conn));
            }
            else {
                _result = PQexecPrepared(conn, _uuid.c_str(), bind.size(),
                                           (const char* const *)param_v, (const int*)param_l, 0, 0);
                delete []param_v;
                delete []param_l;

                pgCheckResult(_result, _sql);
                _rows = (unsigned int)PQNTUPLES(_result);
            }

            // will return 0 for async queries.
            return _rows;
        }

        bool consumeResult() {
            PQconsumeInput(conn);
            return (PQisBusy(conn) ? true : false);
        }

        void prepareResult() {
            PGresult *response;
            _result = PQgetResult(conn);
            _rows   = (unsigned int)PQNTUPLES(_result);
            while ((response = PQgetResult(conn))) PQclear(response);
            pgCheckResult(_result, _sql);
        }

        unsigned long lastInsertID() {
            checkReady("lastInsertID()");
            ResultRow r = fetchRow();
            return r.size() > 0 ? atol(r[0].value.c_str()) : 0;
        }

        ResultRow& fetchRow() {
            checkReady("fetchRow()");

            _rsrow.clear();

            if (_rowno < _rows) {
                _rowno++;
                for (unsigned int i = 0; i < _cols; i++)
                    _rsrow.push_back(PQgetisnull(_result, _rowno-1, i) ?
                        PARAM(null()) : PG2PARAM(_result, _rowno-1, i));
            }

            return _rsrow;
        }

        ResultRowHash& fetchRowHash() {
            checkReady("fetchRowHash()");

            _rsrowhash.clear();

            if (_rowno < _rows) {
                _rowno++;
                for (unsigned int i = 0; i < _cols; i++)
                    _rsrowhash[_rsfields[i]] = PQgetisnull(_result, _rowno-1, i) ?
                        PARAM(null()) : PG2PARAM(_result, _rowno-1, i);
            }

            return _rsrowhash;
        }

        vector<string> fields() {
            return _rsfields;
        }

        unsigned int columns() {
            return _cols;
        }

        bool finish() {
            if (_result)
                PQclear(_result);

            _rowno  = 0;
            _rows   = 0;
            _result = 0;

            return true;
        }

        unsigned char* fetchValue(unsigned int r, unsigned int c, unsigned long *l = 0) {
            checkReady("fetchValue()");
            if (l) *l = PQgetlength(_result, r, c);
            return PQgetisnull(_result, r, c) ? 0 : (unsigned char*)PQgetvalue(_result, r, c);
        }

        unsigned int currentRow() {
            return _rowno;
        }

        void advanceRow() {
            _rowno = _rowno <= _rows ? _rowno + 1 : _rowno;
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
            cleanup();
        }

        void cleanup() {
            // This gets called only on dlclose, so the wrapper dbi::Handle
            // closes connections and frees memory.
            if (conn)
                PQfinish(conn);
            conn = 0;
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

        int socket() {
            return PQsocket(conn);
        }

        PgStatement* aexecute(string sql, vector<Param> &bind) {
            PgStatement *st = new PgStatement(sql, this->conn, true);
            st->execute(bind);
            return st;
        }

        void initAsync() {
            if(!PQisnonblocking(conn)) PQsetnonblocking(conn, 1);
        }

        bool isBusy() {
            return PQisBusy(conn) ? true : false;
        }

        bool cancel() {
            int rc;
            char error[512];

            PGcancel *cancel = PQgetCancel(conn);
            if (!cancel)
                throw RuntimeError("Invalid handle or nothing to cancel");

            rc = PQcancel(cancel, error, 512);
            PQfreeCancel(cancel);

            if (rc != 1)
                throw RuntimeError(error);
            else
                return true;
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
            if (tr_nesting == 0)
                begin();
            execute("SAVEPOINT " + name);
            tr_nesting++;
            return true;
        };

        bool commit(string name) {
            execute("RELEASE SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1)
                commit();
            return true;
        };

        bool rollback(string name) {
            execute("ROLLBACK TO SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1)
                rollback();
            return true;
        };

        void* call(string name, void* args) {
            return NULL;
        }

        bool close() {
            if (conn) PQfinish(conn);
            conn = 0;
            return true;
        }
    };
}

using namespace std;
using namespace dbi;

extern "C" {
    PgHandle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        if (host == "")
            host = "127.0.0.1";
        if (port == "0" || port == "")
            port = "5432";

        return new PgHandle(user, pass, dbname, host, port);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
