#ifndef _DBICXX_MYSQL_RESULT_H
#define _DBICXX_MYSQL_RESULT_H

namespace dbi {
    class MySqlResult : public AbstractResult {
        private:
        uint32_t _rows;
        uint32_t _cols;
        uint32_t _rowno;
        uint32_t _affected_rows;

        vector<string> _rsfields;
        vector<int>    _rstypes;

        MYSQL_ROW _rowdata;
        unsigned long* _rowdata_lengths;

        protected:
        MYSQL *conn;
        MYSQL_RES *result;
        uint64_t last_insert_id;

        void fetchMeta(MYSQL_RES* result);

        public:
        MySqlResult(MYSQL_RES*, string, MYSQL*);
        ~MySqlResult();

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
        void     lastInsertID(uint64_t);

        void cleanup();
        bool finish();
    };
}

#endif
