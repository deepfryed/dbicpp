#ifndef _DBICXX_SQLITE3_RESULT_H
#define _DBICXX_SQLITE3_RESULT_H

namespace dbi {
    class Sqlite3Result : public AbstractResult {

        private:
        string            _sql;
        bool              _lazy_typed;
        uint32_t          _rowno, _rows, _cols;
        int_list_t        _rstypes;
        string_list_t     _rsfields;
        vector<ResultRow> _rowdata;

        void init();
        void fetchMeta(sqlite3_stmt*);

        public:
        uint32_t affected_rows;
        uint64_t last_insert_id;

        Sqlite3Result(sqlite3_stmt *stmt, string sql);
        ~Sqlite3Result();

        void cleanup();
        bool consumeResult();
        void prepareResult();

        uint32_t        rows();
        uint32_t        columns();
        uint64_t        lastInsertID();
        string_list_t&  fields();
        int_list_t&     types();

        bool           read(ResultRow &r);
        bool           read(ResultRowHash &r);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);

        bool     finish();
        uint32_t tell();
        void     rewind();
        void     seek(uint32_t);

        void write(int c, unsigned char *data, uint64_t length);
        void flush(sqlite3_stmt*);
        void clear();
    };
}

#endif
