#ifndef _DBICXX_MYSQL_HANDLE_H
#define _DBICXX_MYSQL_HANDLE_H

namespace dbi {
    class MySqlStatement;
    class MySqlResult;

    class MySqlHandle : public AbstractHandle {
        private:
        string _db;
        string _host;
        string _sql;
        MYSQL_RES *_result;

        protected:
        int tr_nesting;
        void boom(const char*);
        void connectionError(const char *msg = 0);
        void runtimeError(const char *msg = 0);
        void parseOptions(char*);

        public:
        MYSQL *conn;
        MySqlHandle();
        MySqlHandle(string user, string pass, string dbname, string host, string port, char *options);
        ~MySqlHandle();

        MySqlStatement* prepare(string sql);

        uint32_t execute(string sql);
        uint32_t execute(string sql, param_list_t &bind);

        MySqlResult* aexecute(string sql);
        MySqlResult* aexecute(string sql, param_list_t &bind);

        AbstractResult* result();

        void async(bool);
        bool cancel();
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);

        int  socket();
        bool close();
        void cleanup();
        void reconnect();

        uint64_t write(string table, field_list_t &fields, IO*);
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *name);
        string escape(string);
        string driver();
    };
}

#endif
