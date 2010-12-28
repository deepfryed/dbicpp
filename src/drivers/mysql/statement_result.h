#ifndef _DBICXX_MYSQL_STATEMENT_RESULT_H
#define _DBICXX_MYSQL_STATEMENT_RESULT_H

namespace dbi {
    class MySqlStatementResult : public AbstractResult {
        private:
        uint32_t _rows;
        uint32_t _cols;
        uint32_t _rowno;
        uint32_t _affected_rows;

        uint64_t last_insert_id;

        vector<string> _rsfields;
        vector<int>    _rstypes;

        string _sql;

        protected:
        struct rowdata {
            unsigned char **data;
            unsigned long *len;
        };
        struct rowdata *resultset;

        void storeResult(MYSQL_STMT*, MYSQL_BIND*);
        void fetchMeta(MYSQL_STMT*);

        public:
        MySqlStatementResult(MYSQL_STMT*, MYSQL_BIND*);
        ~MySqlStatementResult();

        void checkReady(string m);
        bool consumeResult();
        void prepareResult();

        uint32_t rows();
        uint32_t columns();

        vector<string>& fields();
        vector<int>&    types();

        bool read(ResultRow &r);
        bool read(ResultRowHash &r);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);

        uint32_t tell();
        void seek(uint32_t);
        void rewind();

        uint64_t lastInsertID();

        void cleanup();
        bool finish();
    };
}

#endif
