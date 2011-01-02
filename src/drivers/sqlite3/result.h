#ifndef _DBICXX_SQLITE3_RESULT_H
#define _DBICXX_SQLITE3_RESULT_H

namespace dbi {
    class Sqlite3Result : public AbstractResult {

        private:
        vector<string> _rsfields;
        vector<int>    _rstypes;
        uint32_t       _rowno, _rows, _cols, _affected_rows;
        string         _sql;

        void init();
        void fetchMeta();

        public:
        Sqlite3Result(sqlite3_stmt *stmt, string sql);
        ~Sqlite3Result();

        void cleanup();
        bool consumeResult();
        void prepareResult();
        void boom(const char *);

        uint32_t        rows();
        uint32_t        columns();
        uint64_t        lastInsertID();
        vector<string>& fields();
        vector<int>&    types();

        bool           read(ResultRow &r);
        bool           read(ResultRowHash &r);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);

        bool     finish();
        uint32_t tell();
        void     rewind();
        void     seek(uint32_t);
    };
}

#endif
