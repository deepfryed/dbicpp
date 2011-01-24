#include "common.h"

namespace dbi {

    PgHandle::PgHandle() {
        tr_nesting = 0;
        _result    = 0;
        conn       = 0;
    }

    string PgHandle::parseOptions(char *options) {
        pcrecpp::RE re("([^ =;]+) *= *([^ =;]+)");
        pcrecpp::StringPiece input(options);

        string option;
        string value;
        string extra;
        while (re.FindAndConsume(&input, &option, &value)) {
            extra += " " + option + "='" + value + "'";
        }

        return extra;
    }

    PgHandle::PgHandle(string user, string pass, string dbname, string host, string port, char *options) {
        tr_nesting = 0;
        _result    = 0;
        conn       = 0;

        char conninfo[4096];
        snprintf(conninfo, 1024, "dbname='%s' user='%s' password='%s' host='%s' port='%s' sslmode='allow'",
            dbname.c_str(), user.c_str(), pass.c_str(), host.c_str(), port.c_str());

        if (options) {
            _connextra = parseOptions(options);
            strncat(conninfo, _connextra.c_str(), 4095-strlen(conninfo));
        }

        conn = PQconnectdb(conninfo);

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
        _pgexecDirect(sql);
    }

    void PgHandle::setTimeZone(char *name) {
        char sql[128];
        snprintf(sql, 127, "set time zone '%s'", name);
        _pgexecDirect(sql);
    }

    PgHandle::~PgHandle() {
        cleanup();
    }

    void PgHandle::cleanup() {
        if (_result) PQclear(_result);
        if (conn)    PQfinish(conn);

        _result = 0;
        conn    = 0;
    }

    PgResult* PgHandle::result() {
        PgResult *instance = 0;
        if (_result) {
            instance = new PgResult(_result, _sql, conn);
            _result  = 0;
        }
        return instance; // needs to be deallocated by user.
    }

    void PgHandle::_pgexecDirect(string sql) {
        if (_result) {
            PQclear(_result);
            _result = 0;
        }
        PGresult *result = PQexec(conn, sql.c_str());
        PQ_CHECK_RESULT(result, sql);
        PQclear(result);
    }

    PGresult* PgHandle::_pgexec(string sql) {
        PGresult *result;
        string normalized_sql = sql;
        _sql = sql;

        PQ_PREPROCESS_QUERY(normalized_sql);
        result = PQexec(conn, normalized_sql.c_str());
        PQ_CHECK_RESULT(result, sql);

        if (_result) PQclear(_result);
        return _result = result;
    }

    PGresult* PgHandle::_pgexec(string sql, vector<Param> &bind) {
        int *param_l, *param_f;
        const char **param_v;
        PGresult *result;
        string normalized_sql = sql;
        _sql = sql;

        PQ_PREPROCESS_QUERY(normalized_sql);
        PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);
        try {
            result = PQexecParams(conn, normalized_sql.c_str(), bind.size(), 0,
                                        (const char* const *)param_v, param_l, param_f, 0);
            PQ_CHECK_RESULT(result, sql);
        } catch (Error &e) {
            delete []param_v;
            delete []param_l;
            delete []param_f;
            throw e;
        }

        delete []param_v;
        delete []param_l;
        delete []param_f;

        if (_result) PQclear(_result);
        return _result = result;
    }

    uint32_t PgHandle::execute(string sql) {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec(sql);
        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    uint32_t PgHandle::execute(string sql, vector<Param> &bind) {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec(sql, bind);
        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    int PgHandle::socket() {
        return PQsocket(conn);
    }

    PgResult* PgHandle::aexecute(string sql) {
        string normalized_sql = sql;
        PQ_PREPROCESS_QUERY(normalized_sql);
        int done = PQsendQuery(conn, normalized_sql.c_str());
        if (!done) boom(PQerrorMessage(conn));
        return new PgResult(0, sql, conn);
    }

    PgResult* PgHandle::aexecute(string sql, vector<Param> &bind) {
        int *param_l, *param_f;
        const char **param_v;
        string normalized_sql = sql;
        PQ_PREPROCESS_QUERY(normalized_sql);
        PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);
        int done = PQsendQueryParams(conn, normalized_sql.c_str(), bind.size(),
                                     0, (const char* const *)param_v, param_l, param_f, 0);

        delete []param_v;
        delete []param_l;
        delete []param_f;
        if (!done) boom(PQerrorMessage(conn));
        return new PgResult(0, sql, conn);
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

        if (rc != 1) boom(error);
        return true;
    }

    PgStatement* PgHandle::prepare(string sql) {
        return new PgStatement(sql, conn);
    }

    bool PgHandle::begin() {
        _pgexecDirect("BEGIN WORK");
        tr_nesting++;
        return true;
    };

    bool PgHandle::commit() {
        _pgexecDirect("COMMIT");
        tr_nesting = 0;
        return true;
    };

    bool PgHandle::rollback() {
        _pgexecDirect("ROLLBACK");
        tr_nesting = 0;
        return true;
    };

    bool PgHandle::begin(string name) {
        if (tr_nesting == 0) {
            begin();
            tr_nesting = 0;
        }
        _pgexecDirect("SAVEPOINT " + name);
        tr_nesting++;
        return true;
    };

    bool PgHandle::commit(string name) {
        _pgexecDirect("RELEASE SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 0)
            commit();
        return true;
    };

    bool PgHandle::rollback(string name) {
        _pgexecDirect("ROLLBACK TO SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 0)
            rollback();
        return true;
    };

    bool PgHandle::close() {
        if (conn) PQfinish(conn);
        conn = 0;
        return true;
    }

    void PgHandle::reconnect() {

        PQreset(conn);
        if (PQstatus(conn) == CONNECTION_BAD) {
            if (tr_nesting == 0) {
                char conninfo[4096];
                snprintf(conninfo, 4096, "dbname='%s' user='%s' password='%s' host='%s' port='%s' sslmode='allow'",
                    PQdb(conn), PQuser(conn), PQpass(conn), PQhost(conn), PQport(conn));

                if (_connextra.size() > 0)
                    strncat(conninfo, _connextra.c_str(), 4095-strlen(conninfo));

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

    void PgHandle::boom(const char* m) {
        if (PQstatus(conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    uint64_t PgHandle::write(string table, FieldSet &fields, IO* io) {
        char sql[4096], *buffer;
        uint64_t nrows, len, buffer_len = 1024*1024;

        if (fields.size() > 0)
            snprintf(sql, 4095, "copy %s (%s) from stdin", table.c_str(), fields.join(", ").c_str());
        else
            snprintf(sql, 4095, "copy %s from stdin", table.c_str());

        if (_trace)
            logMessage(_trace_fd, sql);

        _pgexecDirect(sql);

        buffer = new char[buffer_len];
        while ((len = io->read(buffer, buffer_len)) > 0) {
            if (PQputCopyData(conn, buffer, len) != 1) {
                delete [] buffer;
                throw RuntimeError(PQerrorMessage(conn));
            }
        }

        delete [] buffer;
        if (PQputCopyEnd(conn, 0) != 1)
                throw RuntimeError(PQerrorMessage(conn));

        PGresult *res = PQgetResult(conn);
        PQ_CHECK_RESULT(res, sql);
        nrows = atol(PQcmdTuples(res));

        if (_result) PQclear(_result);
        _result = res;

        return nrows;
    }

    string PgHandle::escape(string value) {
        int error;
        char *dest = new char[value.length()*2 + 1];
        PQescapeStringConn(conn, dest, value.data(), value.length(), &error);
        string escaped(dest);
        delete [] dest;
        if (error)
            throw RuntimeError(PQerrorMessage(conn));
        return escaped;
    }

    string PgHandle::driver() {
        return DRIVER_NAME;
    }

}
