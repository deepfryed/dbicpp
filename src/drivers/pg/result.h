#include "src/drivers/pg/common.h"

namespace dbi {
    class PgResult : public AbstractResult {

        private:
        PgHandle       *_handle
        PGconn         *_conn;
        PGresult       *_result;
        vector<string> _rsfields;
        vector<int>    _rstypes;
        uint32_t       _rowno, _rows, _cols;
        bool           _async;
        unsigned char  *_bytea;

        void fetchMeta(PGresult*);
        unsigned char* unescapeBytea(int, int, uint64_t*);

        public:
        PgResult(PGresult*);
        PgResult(PGresult*, PgHandle*);
        ~PgStatement();

        void cleanup();
        bool consumeResult();
        void prepareResult();

        uint32_t       rows();
        uint32_t       columns();
        uint64_t       lastInsertID();
        vector<string> fields();
        vector<int>&   types();

        bool           read(ResultRow &r);
        bool           read(ResultRowHash &r);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);

        bool     finish();
        uint32_t tell();
        void     rewind();
        void     seek(uint32_t);
    };
}
