#ifndef _DBICXX_SQLITE3_HANDLE_H
#define _DBICXX_SQLITE3_HANDLE_H

namespace dbi {

    class Sqlite3Result;
    class Sqlite3Handle : public AbstractHandle {
        private:
        string _sql;
        Sqlite3Result *_result;
        string _dbname;

        void _execute(string);

        protected:
        int tr_nesting;

        public:
        sqlite3 *conn;

        Sqlite3Handle();
        Sqlite3Handle(string dbname);
        ~Sqlite3Handle();
        void cleanup();

        Sqlite3Statement* prepare(string sql);
        uint32_t execute(string sql);
        uint32_t execute(string sql, param_list_t &bind);

        Sqlite3Result* result();

        int  socket();
        void initAsync();
        void async(bool);
        bool isBusy();
        bool cancel();

        Sqlite3Result* aexecute(string sql);
        Sqlite3Result* aexecute(string sql, param_list_t &bind);

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
