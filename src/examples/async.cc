#include "dbic++.h"
#include "dbic++/reactor.h"
#include <unistd.h>

/*------------------------------------------------------------------------------

   To compile:

   g++ -Iinc -Llib -rdynamic -o async async.cc \
       -ldbic++ -ldl -lpcrecpp -levent -lpthread -luuid

------------------------------------------------------------------------------*/

using namespace std;
using namespace dbi;

void callback(AbstractResultSet *res) {
    ResultRowHash r;
    while (res->read(r))
        cout << r << endl;
}

int main(int argc, char *argv[]) {
    string driver(argc > 1 ? argv[1] : "postgresql");

    // Handle h (driver, user, password, database, host, port);
    Handle h (driver, getlogin(), "", "dbicpp");

    // Set trace on and log queries to stderr
    trace(true, fileno(stderr));

    // create table.
    cout << endl;
    cout << "-- creating table --" << endl;

    // create test table
    h.execute("drop table if exists users");
    h.execute("create table users(id serial, name text, email text, primary key (id))");

    // create table.
    cout << endl;
    cout << "-- inserting test data --" << endl;

    // insert some test data
    Statement ins (h, "insert into users(name, email) values(?, ?)");
    ins % "Apple Arthurton", "apple@example.com", execute();
    ins % "Benny Arthurton", "benny@example.com", execute();

    // async querying.
    cout << endl;
    cout << "-- asynchronous query --" << endl;

    string sleep_sql = (driver == "mysql" ? "select sleep" : "select pg_sleep");

    ConnectionPool pool(5, driver, getlogin(), "", "dbicpp");
    Reactor::initialize();
    Reactor::watch(pool.execute(sleep_sql + "(0.5), 1 as query_seq, * from users", callback));
    Reactor::watch(pool.execute(sleep_sql + "(0.3), 2 as query_seq, * from users", callback));
    Reactor::watch(pool.execute(sleep_sql + "(0.1), 3 as query_seq, * from users", callback));
    Reactor::run();
}
