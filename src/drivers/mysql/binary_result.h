#ifndef _DBICXX_MYSQL_BINARY_RESULT_H
#define _DBICXX_MYSQL_BINARY_RESULT_H

#include <mysql/my_global.h>
#include <mysql/mysql_time.h>
#include <mysql/m_string.h>
#include <stdint.h>

namespace dbi {
    struct ResultBuffer {
        unsigned char *data;
        uint64_t length, alloc;
        bool isnull;
    };

    class MySqlBinaryResult : public AbstractResult {
        private:
        vector<string>  _rsfields;
        vector<int>     _rstypes;
        uint32_t _rows, _cols, _rowno, _affected_rows;

        protected:
        MYSQL_RES  *result;
        MYSQL_ROWS *mysqldata, *cursor;

        ResultBuffer *pool;

        uint64_t last_insert_id;

        /* following methods have been stolen from mysql codebase for parsing binary resultset */

        void set_zero_time(MYSQL_TIME*, enum enum_mysql_timestamp_type);
        unsigned long STDCALL net_field_length(unsigned char**);

        void read_binary_time(MYSQL_TIME*, unsigned char **);
        void read_binary_datetime(MYSQL_TIME*, unsigned char **);
        void read_binary_date(MYSQL_TIME*, unsigned char **);

        void fetch_result_tinyint(int, unsigned char **);
        void fetch_result_short(int, unsigned char**);
        void fetch_result_int32(int, unsigned char**);
        void fetch_result_int64(int, unsigned char**);
        void fetch_result_float(int, unsigned char**);
        void fetch_result_double(int, unsigned char**);
        void fetch_result_time(int, unsigned char**);
        void fetch_result_date(int, unsigned char**);
        void fetch_result_datetime(int, unsigned char**);
        void fetch_result_bin(int, unsigned char **);

        /* end of stolen mysql code */

        void fetchRow();
        void fetchMeta();

        public:

        MySqlBinaryResult(MYSQL_STMT *);
        ~MySqlBinaryResult();

        uint32_t        rows();
        uint32_t        columns();
        vector<string>& fields();
        vector<int>&    types();
        uint64_t        lastInsertID();

        bool read(ResultRow &);
        bool read(ResultRowHash &);
        unsigned char* read(uint32_t r, uint32_t c, unsigned long *len);

        uint32_t tell();
        void seek(uint32_t);
        void rewind();

        void cleanup();
        bool finish();

        // NOP
        bool consumeResult();
        void prepareResult();
    };
}

#endif
