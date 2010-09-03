#ifndef _DBICXX_REACTOR_H
#define _DBICXX_REACTOR_H

#include "dbic++.h"
#include <event.h>

namespace dbi {
    static bool _reactorInitialized;

    /*
        Class: Reactor
        Simple libevent based reactor pattern to handle async queries.

        Example:

        (begin code)

        #include "dbic++.h"
        #include <unistd.h>

        using namespace std;
        using namespace dbi;

        void callback(AbstractResult *res) {
            ResultRow r;
            while (res->read(r))
                cout << r << endl;
        }

        int main(int argc, char *argv[]) {
            ConnectionPool pool(5, "postgresql", getlogin(), "", "dbicpp");

            Reactor::initialize();

            Reactor::watch(pool.execute("select pg_sleep(0.5), 1 as query_seq, * from users", callback));
            Reactor::watch(pool.execute("select pg_sleep(0.3), 2 as query_seq, * from users", callback));
            Reactor::watch(pool.execute("select pg_sleep(0.1), 3 as query_seq, * from users", callback));

            // Reactor will terminate after all queries are run and callbacks return.
            Reactor::run();
        }

        (end)
    */
    class Reactor {
        public:
        /*
            Function: run
            Starts the reactor.
        */
        static void run();
        /*
            Function: stop
            Stops the reactor.
        */
        static void stop();
        /*
            Function: initialize
            Setup libevent environment.
        */
        static void initialize();
        /*
            Function: watch(Request*)
            Watches a request dispatched to ConnectionPool

            Parameters:
            request - Pointer to Request returned. See <ConnectionPool>
        */
        static void watch(Request *);
        static void callback(int fd, short type, void *arg);
    };
}

#endif
