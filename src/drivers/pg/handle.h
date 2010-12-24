#include "src/drivers/pg/common.h"

namespace dbi {

    class PgHandle : public AbstractHandle {
        private:
        string _sql;

        protected:
        PGconn *conn;
        int tr_nesting;
        void boom(const char *);

        public:
        PgHandle();
        PgHandle(string user, string pass, string dbname, string h, string p);
        ~PgHandle();
        void cleanup();

        PgStatement* prepare(string sql);
        uint32_t     execute(string sql);
        uint32_t     execute(string sql, vector<Param> &bind);
        PgResult*    query(string sql);
        PgResult*    query(string sql, vector<Param> &bind);

        int  socket();
        void initAsync();
        bool isBusy();
        bool cancel();

        uint32_t  aexecute(string sql, vector<Param> &bind);
        PgResult* aquery(string sql, vector<Param> &bind);

        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);

        bool close();
        void reconnect();

        uint64_t write(string table, FieldSet &fields, IO*);
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *);
        string escape(string);
        string driver();
    };
}
