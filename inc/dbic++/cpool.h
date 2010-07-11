#ifndef _DBICXX_CPOOL_H
#define _DBICXX_CPOOL_H

namespace dbi {

    class AbstractHandle;
    class AbstractResultSet;
    class Request;

    class ConnectionPool {
        public:
        ConnectionPool(int size, string driver, string user, string pass, string dbname, string host, string port);
        ConnectionPool(int size, string driver_name, string user, string pass, string dbname);
        ~ConnectionPool();

        Request* execute(string sql, vector<Param> &bind, void (*callback)(AbstractResultSet *r), void *context = 0);
        Request* execute(string sql, void (*callback)(AbstractResultSet *r), void *context = 0);
        bool process(Request *);

        protected:
        struct Connection {
            AbstractHandle* handle;
            bool busy;
        };
        vector<struct Connection> pool;
    };

    class Request {
        public:
        Request(ConnectionPool *, AbstractHandle *, AbstractResultSet *, void (*)(AbstractResultSet *));
        ~Request();
        int socket();
        bool process();
        protected:
        ConnectionPool *target;
        AbstractHandle *handle;
        AbstractResultSet *result;
        void (*callback)(AbstractResultSet *r);

        friend class ConnectionPool;
    };
}

#endif
