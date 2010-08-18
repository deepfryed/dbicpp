#ifndef _DBICXX_CPOOL_H
#define _DBICXX_CPOOL_H

namespace dbi {

    class AbstractHandle;
    class AbstractResultSet;
    class Request;

    /*
        Class: ConnectionPool
        The dbic++ connection pool is a set of database connections that can be accessed in an asynchronous manner.

        See <Reactor> for a working example or src/examples/async.cc in dbic++ source distribution.
    */

    /*
        Typedef: Callback
        A callback function passed to ConnectionPool::execute()

        (begin code)
        typedef void (*Callback)(AbstractResultSet *r);
        (end)
    */
    typedef void (*Callback)(AbstractResultSet *r);

    class ConnectionPool {
        public:
        /*
            Constructor: ConnectionPool(int, string, string, string, string, string, string)
            Creates a connection pool of a given size.

            Parameters:
            size   - Pool size (this opens multiple connections)
            driver - Database driver name ("postgresql", "mysql", etc.)
            user   - Database user name
            pass   - Database password
            dbname - Database name
            host   - Host name or ip address.
            port   - Port number as string.
        */
        ConnectionPool(int size, string driver, string user, string pass, string dbname, string host, string port);
        /*
            Constructor: ConnectionPool(int, string, string, string, string)
            Creates a connection pool of a given size and connects to localhost on the default port.

            Parameters:
            size   - Pool size (this opens multiple connections)
            driver - Database driver name ("postgresql", "mysql", etc.)
            user   - Database user name
            pass   - Database password
            dbname - Database name
        */
        ConnectionPool(int size, string driver, string user, string pass, string dbname);
        ~ConnectionPool();

        /*
            Function: execute(string, ResultRow&, Callback, void *)
            Executes the given SQL using the bind parameters and invokes the callback when
            results are ready.

            Parameters:
            sql      - SQL
            bind     - Bind params
            callback - A callback that is to be invoked with the result set.
            context  - A void pointer to any additional context information you need along
                       with the results (optional).
        */
        Request* execute(string sql, ResultRow &bind, Callback callback, void *context = 0);
        /*
            Function: execute(string, Callback, void *)
            Executes the given SQL and invokes the callback when results are ready.

            Parameters:
            sql      - SQL
            callback - A callback that is to be invoked with the result set.
            context  - A void pointer to any additional context information you need along
                       with the results (optional).
        */
        Request* execute(string sql, Callback callback, void *context = 0);

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
