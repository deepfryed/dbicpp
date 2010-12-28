#include "common.h"

namespace dbi {
    void PgStatement::init() {
        _uuid   = generateCompactUUID();
        _result = 0;
    }

    void PgStatement::prepare() {
        PGresult *result = 0;
        string normalized_sql = _sql;
        PQ_PREPROCESS_QUERY(normalized_sql);

        result = PQprepare(_conn, _uuid.c_str(), normalized_sql.c_str(), 0, 0);
        if (!result) boom("Unable to allocate statement");

        PQ_CHECK_RESULT(result, _sql);
        PQclear(result);
    }

    PgStatement::~PgStatement() { cleanup(); }
    PgStatement::PgStatement(string normalized_sql,  PGconn *conn) {
        _sql  = normalized_sql;
        _conn = conn;

        init();
        prepare();
    }

    void PgStatement::cleanup() {
        char command[1024];
        if (_result) { PQclear(_result); _result = 0; }
        if (_uuid.length() == 32 && PQstatus(_conn) != CONNECTION_BAD) {
            snprintf(command, 1024, "deallocate \"%s\"", _uuid.c_str());
            PQexec(_conn, command);
        }
    }

    void PgStatement::finish() {
        if (_result) { PQclear(_result); _result = 0; }
    }

    string PgStatement::command() {
        return _sql;
    }

    PGresult* PgStatement::_pgexec() {
        PGresult *result;
        result = PQexecPrepared(_conn, _uuid.c_str(), 0, 0, 0, 0, 0);
        PQ_CHECK_RESULT(result, _sql);
        if (_result) PQclear(_result);
        return _result = result;
    }

    PGresult* PgStatement::_pgexec(vector<Param> &bind) {
        int *param_l, *param_f;
        const char **param_v;
        PGresult *result;

        if (bind.size() == 0) return _pgexec();

        PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);

        try {
            result = PQexecPrepared(_conn, _uuid.c_str(), bind.size(),
                                   (const char* const *)param_v, (const int*)param_l, (const int*)param_f, 0);
            PQ_CHECK_RESULT(result, _sql);
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

    uint32_t PgStatement::execute() {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec();

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    uint32_t PgStatement::execute(vector<Param> &bind) {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec(bind);

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        return ctuples > 0 ? ctuples : rows;
    }

    PgResult* PgStatement::result() {
        PgResult *instance = 0;
        if (_result) {
            instance = new PgResult(_result, _sql, 0);
            _result  = 0;
        }
        return instance; // needs to be deallocated by user.
    }

    void PgStatement::boom(const char* m) {
        if (PQstatus(_conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    uint64_t PgStatement::lastInsertID() {
        if (_result) {
            if (PQgetisnull(_result, 0, 0))
                return 0;
            else
                return atol(PQgetvalue(_result, 0, 0));
        }
        return 0;
    }
}
