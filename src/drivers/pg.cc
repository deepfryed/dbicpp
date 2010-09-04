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

    char errormsg[8192];

    const char *typemap[] = {
        "%d", "int", "%u", "int", "%lu", "int", "%ld", "int",
        "%f", "float", "%lf", "float",
        "%s", "text"
    };

    void PQ_NOTICE(void *arg, const char *message) {
        if (_trace) logMessage(_trace_fd, message);
    }

    void PQ_PREPROCESS_QUERY(string &query) {
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

    void PQ_PROCESS_BIND(const char ***param_v, int **param_l, int **param_f, vector<Param> &bind) {
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


    // ----------------------------------------------------------------------
    // Statement & Handle
    // ----------------------------------------------------------------------

    class PgHandle;
    class PgStatement : public AbstractStatement {
        private:
        string _sql;
        string _uuid;
        PGresult *_result;
        vector<string> _rsfields;
        vector<int> _rstypes;
        uint32_t _rowno, _rows, _cols;
        bool _async;
        unsigned char *_bytea;

        protected:
        PgHandle *handle;
        void init();
        PGresult* prepare();
        void boom(const char *);
        void fetchMeta(PGresult *);
        unsigned char* unescapeBytea(int, int, uint64_t*);

        public:
        PgStatement();
        ~PgStatement();
        PgStatement(string query, PgHandle *h, bool async = false);
        PgStatement(string query, PgHandle *h, PGresult *result);
        void cleanup();
        string command();
        void checkReady(string);
        uint32_t rows();
        uint32_t columns();
        uint32_t execute();
        uint32_t execute(vector<Param>&);
        bool consumeResult();
        void prepareResult();
        uint64_t lastInsertID();
        bool read(ResultRow &r);
        bool read(ResultRowHash &r);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);
        vector<string> fields();
        bool finish();
        uint32_t tell();
        void rewind();
        vector<int>& types();
        void seek(uint32_t);
    };

    class PgHandle : public AbstractHandle {
        private:
        PGresult *_result;
        string _sql;
        protected:
        PGconn *conn;
        int tr_nesting;
        void boom(const char *);
        public:
        PgHandle();
        PgHandle(string user, string pass, string dbname, string h, string p);
        ~PgHandle();
        void cleanup();
        uint32_t execute(string sql);
        uint32_t execute(string sql, vector<Param> &bind);
        int socket();
        PgStatement* aexecute(string sql, vector<Param> &bind);
        void initAsync();
        bool isBusy();
        bool cancel();
        PgStatement* prepare(string sql);
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);
        void* call(string name, void* args, uint64_t l);
        bool close();
        void reconnect(bool barf = false);
        int checkResult(PGresult*, string, bool barf = false);
        uint64_t write(string table, FieldSet &fields, IO*);
        AbstractResultSet* results();
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *);
        string escape(string);

        friend class PgStatement;
    };


    // ----------------------------------------------------------------------
    // DEFINITION
    // ----------------------------------------------------------------------

    void PgStatement::init() {
        _result = 0;
        _rowno  = 0;
        _rows   = 0;
        _cols   = 0;
        _uuid   = generateCompactUUID();
        _async  = false;
        _bytea  = 0;
    }

    PGresult* PgStatement::prepare() {
        int tries, done;
        PGresult *response, *result;
        string query = _sql;
        PQ_PREPROCESS_QUERY(query);

        if (_async) {
            done = tries = 0;
            while (!done && tries < 2) {
                tries++;
                done = PQsendPrepare(handle->conn, _uuid.c_str(), query.c_str(), 0, 0);
                if (!done) { handle->reconnect(true); continue; }

                while (PQisBusy(handle->conn)) PQconsumeInput(handle->conn);
                result = PQgetResult(handle->conn);
                while ((response = PQgetResult(handle->conn))) PQclear(response);

                done = handle->checkResult(result, _sql);
                PQclear(result);
                if (!done) continue;

                done = PQsendDescribePrepared(handle->conn, _uuid.c_str());
                if (!done) continue;

                while (PQisBusy(handle->conn)) PQconsumeInput(handle->conn);
                result = PQgetResult(handle->conn);
                while ((response = PQgetResult(handle->conn))) PQclear(response);
                done = handle->checkResult(result, _sql);
            }

            if (!done) boom(PQerrorMessage(handle->conn));
        }
        else {
            done = tries = 0;
            while (!done && tries < 2) {
                tries++;
                result = PQprepare(handle->conn, _uuid.c_str(), query.c_str(), 0, 0);
                if (!result) boom("Unable to allocate statement");
                done = handle->checkResult(result, _sql);
                PQclear(result);

                if (!done) continue;

                result = PQdescribePrepared(handle->conn, _uuid.c_str());
                done = handle->checkResult(result, _sql);
            }

            if (!done) boom(PQerrorMessage(handle->conn));
        }

        return result;
    }

    PgStatement::~PgStatement() { cleanup(); }
    PgStatement::PgStatement(string query,  PgHandle *h, bool async) {
        init();
        _sql   = query;
        _async = async;
        handle = h;

        // prepare is always sync - only execute is really async.
        PGresult *result = prepare();
        fetchMeta(result);
        PQclear(result);
    }

    PgStatement::PgStatement(string query, PgHandle *h, PGresult *result) {
        init();
        _uuid   = "";
        _sql    = query;
        handle  = h;
        _result = result;
        _rows   = PQntuples(result);
        fetchMeta(_result);
    }

    void PgStatement::fetchMeta(PGresult *result) {
        _cols  = (uint32_t)PQnfields(result);

        for (int i = 0; i < (int)_cols; i++)
            _rsfields.push_back(PQfname(result, i));

        // postgres types are all in pg_type. no clue why they're not in the headers.
        for (int i = 0; i < (int)_cols; i++) {
            switch(PQftype(result, i)) {
                case   16: _rstypes.push_back(DBI_TYPE_BOOLEAN); break;
                case   17: _rstypes.push_back(DBI_TYPE_BLOB); break;
                case   20:
                case   21:
                case   23: _rstypes.push_back(DBI_TYPE_INT); break;
                case   25: _rstypes.push_back(DBI_TYPE_TEXT); break;
                case  700:
                case  701: _rstypes.push_back(DBI_TYPE_FLOAT); break;
                case 1082: _rstypes.push_back(DBI_TYPE_DATE); break;
                case 1114:
                case 1184: _rstypes.push_back(DBI_TYPE_TIME); break;
                case 1700: _rstypes.push_back(DBI_TYPE_NUMERIC); break;
                  default: _rstypes.push_back(DBI_TYPE_TEXT); break;
            }
        }
    }

    void PgStatement::cleanup() {
        finish();
        if (_bytea)
            PQfreemem(_bytea);
        // NOTE dont think we need to deallocate prepared statements. Just playing nice.
        if (_cols > 0 && _uuid.length() == 32 && PQstatus(handle->conn) != CONNECTION_BAD)
            handle->execute("deallocate \"" + _uuid + "\"");
    }

    string PgStatement::command() {
        return _sql;
    }

    void PgStatement::checkReady(string m) {
        if (!_result)
            throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
    }

    uint32_t PgStatement::rows() {
        return _rows;
    }

    uint32_t PgStatement::execute() {
        uint32_t ctuples = 0;
        int done, tries;

        finish();

        if (_async) {
            done = tries = 0;
            while (!done && tries < 2) {
                tries++;
                done = PQsendQueryPrepared(handle->conn, _uuid.c_str(), 0, 0, 0, 0, 0);
                if (!done) handle->reconnect(true);
            }
            if (!done) boom(PQerrorMessage(handle->conn));
        }
        else {
            done = tries = 0;
            while (!done && tries < 2) {
                tries++;
                _result = PQexecPrepared(handle->conn, _uuid.c_str(), 0, 0, 0, 0, 0);
                done = handle->checkResult(_result, _sql);
                if (!done) prepare();
            }
            _rows   = (uint32_t)PQntuples(_result);
            ctuples = (uint32_t)atoi(PQcmdTuples(_result));
        }

        return ctuples > 0 ? ctuples : _rows;
    }

    uint32_t PgStatement::execute(vector<Param> &bind) {
        uint32_t ctuples = 0;
        int *param_l, *param_f, done, tries;
        const char **param_v;

        finish();
        PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);

        if (_async) {
            done = tries = 0;
            while (!done && tries < 2) {
                tries++;
                done = PQsendQueryPrepared(handle->conn, _uuid.c_str(), bind.size(),
                                       (const char* const *)param_v, param_l, param_f, 0);
            }
            delete []param_v;
            delete []param_l;
            delete []param_f;

            if (!done) boom(PQerrorMessage(handle->conn));
        }
        else {
            done = tries = 0;
            try {
                while (!done && tries < 2) {
                    tries++;
                    _result = PQexecPrepared(handle->conn, _uuid.c_str(), bind.size(),
                                       (const char* const *)param_v, (const int*)param_l, (const int*)param_f, 0);
                    done = handle->checkResult(_result, _sql);
                    if (!done) prepare();
                }
            } catch (Error &e) {
                delete []param_v;
                delete []param_l;
                delete []param_f;
                throw e;
            }

            delete []param_v;
            delete []param_l;
            delete []param_f;

            _rows   = (uint32_t)PQntuples(_result);
            ctuples = (uint32_t)atoi(PQcmdTuples(_result));
        }

        return ctuples > 0 ? ctuples : _rows;
    }

    bool PgStatement::consumeResult() {
        PQconsumeInput(handle->conn);
        return (PQisBusy(handle->conn) ? true : false);
    }

    void PgStatement::prepareResult() {
        PGresult *response;
        _result = PQgetResult(handle->conn);
        _rows   = (uint32_t)PQntuples(_result);
        while ((response = PQgetResult(handle->conn))) PQclear(response);
        handle->checkResult(_result, _sql, true);
    }

    uint64_t PgStatement::lastInsertID() {
        ResultRow r;
        return read(r) && r.size() > 0 ? atol(r[0].value.c_str()) : 0;
    }

    unsigned char* PgStatement::unescapeBytea(int r, int c, uint64_t *l) {
        size_t len;
        if (_bytea) PQfreemem(_bytea);
        _bytea = PQunescapeBytea((unsigned char *)PQgetvalue(_result, r, c), &len);
        *l = (uint64_t)len;
        return _bytea;
    }

    bool PgStatement::read(ResultRow &row) {
        uint64_t len;
        unsigned char *data;

        row.clear();

        if (_rowno < _rows) {
            for (uint32_t i = 0; i < _cols; i++) {
                if (PQgetisnull(_result, _rowno, i))
                    row.push_back(PARAM(null()));
                else if (_rstypes[i] != DBI_TYPE_BLOB)
                    row.push_back(PG2PARAM(_result, _rowno, i));
                else {
                    data = unescapeBytea(_rowno, i, &len);
                    row.push_back(PARAM(data, len));
                }
            }
            _rowno++;
            return true;
        }

        return false;
    }

    bool PgStatement::read(ResultRowHash &rowhash) {
        uint64_t len;
        unsigned char *data;

        rowhash.clear();

        if (_rowno < _rows) {
            for (uint32_t i = 0; i < _cols; i++) {
                if (PQgetisnull(_result, _rowno, i))
                    rowhash[_rsfields[i]] = PARAM(null());
                else if (_rstypes[i] != DBI_TYPE_BLOB)
                    rowhash[_rsfields[i]] = PG2PARAM(_result, _rowno, i);
                else {
                    data = unescapeBytea(_rowno, i, &len);
                    rowhash[_rsfields[i]] = PARAM(data, len);
                }
            }
            _rowno++;
            return true;
        }

        return false;
    }

    vector<string> PgStatement::fields() {
        return _rsfields;
    }

    uint32_t PgStatement::columns() {
        return _cols;
    }

    bool PgStatement::finish() {
        if (_result)
            PQclear(_result);
        _rowno  = 0;
        _rows   = 0;
        _result = 0;

        return true;
    }

    unsigned char* PgStatement::read(uint32_t r, uint32_t c, uint64_t *l) {
        _rowno = r;
        if (PQgetisnull(_result, r, c)) {
            return 0;
        }
        else if (_rstypes[c] != DBI_TYPE_BLOB) {
            if (l) *l = PQgetlength(_result, r, c);
            return (unsigned char*)PQgetvalue(_result, r, c);
        }
        else {
            return unescapeBytea(r, c, l);
        }
    }

    uint32_t PgStatement::tell() {
        return _rowno;
    }

    void PgStatement::boom(const char* m) {
        if (PQstatus(handle->conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    void PgStatement::rewind() {
        _rowno = 0;
    }

    void PgStatement::seek(uint32_t r) {
        _rowno = r;
    }

    vector<int>& PgStatement::types() {
        return _rstypes;
    }

    PgHandle::PgHandle() { tr_nesting = 0; }
    PgHandle::PgHandle(string user, string pass, string dbname, string h, string p) {
        _result    = 0;
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

        PQsetNoticeProcessor(conn, PQ_NOTICE, 0);
        PQsetClientEncoding(conn, "utf8");
    }

    void PgHandle::setTimeZoneOffset(int tzhour, int tzmin) {
        char sql[128];
        // server parses TZ style format. man timzone for more info.
        snprintf(sql, 127, "set time zone 'UTC%+02d:%02d'", -1* tzhour, abs(tzmin));
        execute(sql);
    }

    void PgHandle::setTimeZone(char *name) {
        char sql[128];
        snprintf(sql, 127, "set time zone '%s'", name);
        execute(sql);
    }

    PgHandle::~PgHandle() {
        cleanup();
    }

    void PgHandle::cleanup() {
        // This gets called only on dlclose, so the wrapper dbi::Handle
        // closes connections and frees memory.
        if (_result)
            PQclear(_result);
        if (conn)
            PQfinish(conn);
        conn    = 0;
        _result = 0;
    }

    uint32_t PgHandle::execute(string sql) {
        uint32_t rows, ctuples = 0;
        int done, tries;
        PGresult *result;
        string query  = sql;
        _sql = sql;

        done = tries = 0;
        PQ_PREPROCESS_QUERY(query);
        while (!done && tries < 2) {
            tries++;
            result = PQexec(conn, query.c_str());
            done = checkResult(result, sql);
        }

        if (!done) boom(PQerrorMessage(conn));

        if (_result) PQclear(_result);
        _result = result;

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    uint32_t PgHandle::execute(string sql, vector<Param> &bind) {
        int *param_l, *param_f;
        const char **param_v;
        uint32_t rows, ctuples = 0;
        int done, tries;
        PGresult *result;
        string query = sql;
        _sql = sql;

        done = tries = 0;
        PQ_PREPROCESS_QUERY(query);
        try {
            while (!done && tries < 2) {
                tries++;
                PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);
                result = PQexecParams(conn, query.c_str(), bind.size(),
                                        0, (const char* const *)param_v, param_l, param_f, 0);
                done = checkResult(result, sql);
            }
        }
        catch (Error &e) {
            delete []param_v;
            delete []param_l;
            delete []param_f;
            throw e;
        }

        delete []param_v;
        delete []param_l;
        delete []param_f;

        if (_result) PQclear(_result);
        _result = result;

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    AbstractResultSet* PgHandle::results() {
        if (_result) {
            PgStatement *st = new PgStatement(_sql, this, _result);
            _result = 0;
            return st;
        }
        return 0;
    }

    int PgHandle::socket() {
        return PQsocket(conn);
    }

    PgStatement* PgHandle::aexecute(string sql, vector<Param> &bind) {
        PgStatement *st = new PgStatement(sql, this, true);
        st->execute(bind);
        return st;
    }

    void PgHandle::initAsync() {
        if(!PQisnonblocking(conn)) PQsetnonblocking(conn, 1);
    }

    bool PgHandle::isBusy() {
        return PQisBusy(conn) ? true : false;
    }

    bool PgHandle::cancel() {
        int rc;
        char error[512];

        PGcancel *cancel = PQgetCancel(conn);
        if (!cancel)
            boom("Invalid handle or nothing to cancel");

        rc = PQcancel(cancel, error, 512);
        PQfreeCancel(cancel);

        if (rc != 1)
            boom(error);
        else
            return true;
    }

    PgStatement* PgHandle::prepare(string sql) {
        return new PgStatement(sql, this);
    }

    bool PgHandle::begin() {
        execute("BEGIN WORK");
        tr_nesting++;
        return true;
    };

    bool PgHandle::commit() {
        execute("COMMIT");
        tr_nesting = 0;
        return true;
    };

    bool PgHandle::rollback() {
        execute("ROLLBACK");
        tr_nesting = 0;
        return true;
    };

    bool PgHandle::begin(string name) {
        if (tr_nesting == 0)
            begin();
        execute("SAVEPOINT " + name);
        tr_nesting++;
        return true;
    };

    bool PgHandle::commit(string name) {
        execute("RELEASE SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 1)
            commit();
        return true;
    };

    bool PgHandle::rollback(string name) {
        execute("ROLLBACK TO SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 1)
            rollback();
        return true;
    };

    void* PgHandle::call(string name, void* args, uint64_t l) {
        return NULL;
    }

    bool PgHandle::close() {
        if (conn) PQfinish(conn);
        conn = 0;
        return true;
    }

    void PgHandle::reconnect(bool barf) {

        if (barf && PQstatus(conn) != CONNECTION_BAD)
            throw ConnectionError(PQerrorMessage(conn));

        PQreset(conn);
        if (PQstatus(conn) == CONNECTION_BAD) {
            if (tr_nesting == 0) {
                char conninfo[8192];
                snprintf(conninfo, 8192, "dbname=%s user=%s password=%s host=%s port=%s",
                    PQdb(conn), PQuser(conn), PQpass(conn), PQhost(conn), PQport(conn));
                PQfinish(conn);
                conn = PQconnectdb(conninfo);
                if (PQstatus(conn) == CONNECTION_BAD) throw ConnectionError(PQerrorMessage(conn));
                sprintf(errormsg, "WARNING: Socket changed during auto reconnect to database %s on host %s\n",
                    PQdb(conn), PQhost(conn));
                logMessage(_trace_fd, errormsg);
            }
            else throw ConnectionError(PQerrorMessage(conn));
        }
        else {
            sprintf(errormsg, "NOTICE: Auto reconnected on same socket to database %s on host %s\n",
                PQdb(conn), PQhost(conn));
            logMessage(_trace_fd, errormsg);
        }
    }


    int PgHandle::checkResult(PGresult *result, string sql, bool barf) {
        switch(PQresultStatus(result)) {
            case PGRES_TUPLES_OK:
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_EMPTY_QUERY:
            case PGRES_COMMAND_OK:
                return 1;
            case PGRES_BAD_RESPONSE:
            case PGRES_FATAL_ERROR:
            case PGRES_NONFATAL_ERROR:
                if (PQstatus(conn) == CONNECTION_BAD && !barf) {
                    reconnect();
                    PQclear(result);
                    return 0;
                }
                snprintf(errormsg, 8192, "In SQL: %s\n\n %s", sql.c_str(), PQresultErrorMessage(result));
                throw RuntimeError((const char*)errormsg);
                break;
            default:
                if (PQstatus(conn) == CONNECTION_BAD && !barf) {
                    reconnect();
                    PQclear(result);
                    return 0;
                }
                snprintf(errormsg, 8192, "In SQL: %s\n\n Unknown error, check logs.", sql.c_str());
                throw RuntimeError(errormsg);
                break;
        }
        return 1;
    }

    void PgHandle::boom(const char* m) {
        if (PQstatus(conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    uint64_t PgHandle::write(string table, FieldSet &fields, IO* io) {
        char sql[4096];
        uint64_t nrows;
        snprintf(sql, 4095, "copy %s (%s) from stdin", table.c_str(), fields.join(", ").c_str());
        if (_trace)
            logMessage(_trace_fd, sql);
        execute(sql);
        string rows = io->read();
        while (rows.length() > 0) {
            if (PQputCopyData(conn, rows.data(), rows.length()) != 1)
                throw RuntimeError(PQerrorMessage(conn));
            rows = io->read();
        }
        if (PQputCopyEnd(conn, 0) != 1)
                throw RuntimeError(PQerrorMessage(conn));
        PGresult *res = PQgetResult(conn);
        checkResult(res, sql, true);
        nrows = atol(PQcmdTuples(res));
        PQclear(res);
        return nrows;
    }

    string PgHandle::escape(string value) {
        int error;
        char *dest = (char *)malloc(value.length()*2 + 1);
        PQescapeStringConn(conn, dest, value.data(), value.length(), &error);
        string escaped(dest);
        free(dest);
        if (error)
            throw RuntimeError(PQerrorMessage(conn));
        return escaped;
    }
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
