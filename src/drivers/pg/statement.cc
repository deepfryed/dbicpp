#include "common.h"

namespace dbi {
    void PgStatement::init() {
        _uuid           = generateCompactUUID();
        _result         = 0;
        _last_insert_id = 0;
    }

    void PgStatement::prepare() {
        PGresult *result = 0;
        string normalized_sql = _sql;
        PQ_PREPROCESS_QUERY(normalized_sql);

        result = PQprepare(*_conn, _uuid.c_str(), normalized_sql.c_str(), 0, 0);
        if (!result) boom("Unable to allocate statement");

        PQ_CHECK_RESULT(result, *_conn, _sql);
        PQclear(result);
    }

    PgStatement::~PgStatement() {
        cleanup();
    }

    PgStatement::PgStatement(string normalized_sql,  PGconn **conn) {
        _sql  = normalized_sql;
        _conn = conn;

        init();
        prepare();
    }

    void PgStatement::cleanup() {
        char command[1024];
        if (_result) { PQclear(_result); _result = 0; }
        if (_uuid.length() == 32 && PQstatus(*_conn) != CONNECTION_BAD) {
            snprintf(command, 1024, "deallocate \"%s\"", _uuid.c_str());
            PQclear(PQexec(*_conn, command));
        }
    }

    void PgStatement::finish() {
        if (_result) { PQclear(_result); _result = 0; }
        _last_insert_id = 0;
    }

    string PgStatement::command() {
        return _sql;
    }

    PGresult* PgStatement::_pgexec() {
        PGresult *result;

        finish();
        result = PQexecPrepared(*_conn, _uuid.c_str(), 0, 0, 0, 0, 0);
        PQ_CHECK_RESULT(result, *_conn, _sql);
        return _result = result;
    }

    PGresult* PgStatement::_pgexec(vector<Param> &bind) {
        int *param_l, *param_f;
        const char **param_v;
        PGresult *result;

        if (bind.size() == 0) return _pgexec();

        finish();
        PQ_PROCESS_BIND(&param_v, &param_l, &param_f, bind);

        try {
            result = PQexecPrepared(*_conn, _uuid.c_str(), bind.size(),
                                   (const char* const *)param_v, (const int*)param_l, (const int*)param_f, 0);
            PQ_CHECK_RESULT(result, *_conn, _sql);
        }
        catch (ConnectionError &e) {
            delete []param_v;
            delete []param_l;
            delete []param_f;
            throw e;
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

        return _result = result;
    }

    uint32_t PgStatement::execute() {
        uint32_t rows;

        PGresult *result = _pgexec();
        rows             = (uint32_t)PQntuples(result);
        _last_insert_id  = rows > 0 ? PQgetisnull(result, 0, 0) ? 0 : atol(PQgetvalue(result, 0, 0)) : 0;

        return rows > 0 ? rows : (uint32_t)atoi(PQcmdTuples(result));
    }

    uint32_t PgStatement::execute(vector<Param> &bind) {
        uint32_t rows;

        PGresult *result = _pgexec(bind);
        rows             = (uint32_t)PQntuples(result);
        _last_insert_id  = rows > 0 ? PQgetisnull(result, 0, 0) ? 0 : atol(PQgetvalue(result, 0, 0)) : 0;

        return rows > 0 ? rows : (uint32_t)atoi(PQcmdTuples(result));
    }

    PgResult* PgStatement::result() {
        PgResult *instance = new PgResult(_result, _sql, *_conn);
        _result            = 0;
        return instance;
    }

    void PgStatement::boom(const char* m) {
        if (PQstatus(*_conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    uint64_t PgStatement::lastInsertID() {
        return _last_insert_id;
    }
}
