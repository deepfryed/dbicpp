#ifndef _DBICXX_PG_STATEMENT_H
#define _DBICXX_PG_STATEMENT_H

namespace dbi {
    class PgStatement : public AbstractStatement {
        private:
        uint64_t _last_insert_id;
        string _sql, _uuid;
        PGconn **_conn;

        PGresult *_result;

        PGresult* _pgexec();
        PGresult* _pgexec(vector<Param>&);

        protected:
        PgHandle *handle;
        void      init();
        void      prepare();
        void      boom(const char *);

        public:
        PgStatement();
        ~PgStatement();
        PgStatement(string query, PGconn **conn);
        void cleanup();
        void finish();
        string command();

        uint32_t execute();
        uint32_t execute(vector<Param>&);
        uint64_t lastInsertID();

        PgResult* result();
    };
}

#endif
