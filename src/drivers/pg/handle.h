#ifndef _DBICXX_PG_HANDLE_H
#define _DBICXX_PG_HANDLE_H

namespace dbi {

    class PgHandle : public AbstractHandle {
        private:
        string _sql;
        PGresult* _pgexec(string sql);
        PGresult* _pgexec(string sql, param_list_t &bind);

        // execute queries directly and don't save results.
        void _pgexecDirect(string sql);

        PGresult *_result;
        string _connextra;

        protected:
        int tr_nesting;
        void boom(const char *);
        string parseOptions(char*);
        string escaped(string);

        public:
        PGconn *conn;

        PgHandle();
        PgHandle(string user, string pass, string dbname, string h, string p, char *options);
        ~PgHandle();
        void cleanup();

        PgStatement* prepare(string sql);
        uint32_t     execute(string sql);
        uint32_t     execute(string sql, param_list_t &bind);

        PgResult*    result();

        int  socket();
        void async(bool);
        bool cancel();

        PgResult* aexecute(string sql);
        PgResult* aexecute(string sql, param_list_t &bind);

        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);

        bool close();
        void reconnect();

        uint64_t write(string table, field_list_t &fields, IO*);
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *);
        string escape(string);
        string driver();
    };
}

#endif
