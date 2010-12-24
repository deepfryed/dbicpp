namespace dbi {
    void PgStatement::init() {
        _uuid = generateCompactUUID();
    }

    void PgStatement::prepare() {
        PGresult *response, *result = 0;
        string query = _sql;
        PQ_PREPROCESS_QUERY(query);

        result = PQprepare(_conn, _uuid.c_str(), query.c_str(), 0, 0);
        if (!result) boom("Unable to allocate statement");

        PQ_CHECK_RESULT(result, _sql);
        PQclear(result);
    }

    PgStatement::~PgStatement() { cleanup(); }
    PgStatement::PgStatement(string query,  PGconn *conn) {
        _sql  = query;
        _conn = conn;

        init();
        prepare();
    }

    void PgStatement::cleanup() {
        char command[1024];
        if (_uuid.length() == 32 && PQstatus(_conn) != CONNECTION_BAD) {
            snprintf(command, 1024, "deallocate \"%s\"", _uuid.c_str());
            PQexec(_conn, command);
        }
    }

    string PgStatement::command() {
        return _sql;
    }

    PGresult* _pgexec() {
        PGresult *result;
        result = PQexecPrepared(_conn, _uuid.c_str(), 0, 0, 0, 0, 0);
        PQ_CHECK_RESULT(result, _sql);
        return result;
    }


    PGresult* _pgexec(vector<Param> &bind) {
        int *param_l, *param_f, done, tries;
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

        return result;
    }

    uint32_t PgStatement::execute() {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec();

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        PQclear(result);
        return ctuples > 0 ? ctuples : rows;
    }

    uint32_t PgStatement::execute(vector<Param> &bind) {
        uint32_t rows, ctuples = 0;
        PGresult *result = _pgexec(bind);

        rows    = (uint32_t)PQntuples(result);
        ctuples = (uint32_t)atoi(PQcmdTuples(result));

        PQclear(result);
        return ctuples > 0 ? ctuples : rows;
    }

    PgResult* query() {
        return new PgResult(_pgexec(), 0);
    }

    PgResult* query(vector<Param> &bind) {
        return new PgResult(_pgexec(bind), 0);
    }

    void PgStatement::boom(const char* m) {
        if (PQstatus(_conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }
}
