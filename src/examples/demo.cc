#include "dbic++.h"
#include "dbic++/reactor.h"
#include <unistd.h>

/*--------------------------------------------------------------------------------------

   To compile:

   g++ -Iinc -Llib -rdynamic -o demo demo.cc -ldbic++ -ldl -lpcrecpp -levent -lpthread

--------------------------------------------------------------------------------------*/

using namespace std;
using namespace dbi;

void printResultRows(Statement &st) {
    ResultRow r;
    while (r = st.fetchRow()) {
        cout << r.join("\t") << endl;
    }
}

void callback(AbstractResultSet *r) {
    for (int i = 0; i < r->rows(); i++) {
        cout << r->fetchRow() << endl;
    }
}

int main(int argc, char *argv[]) {
    string driver(argc > 1 ? argv[1] : "postgresql");

    // Handle h ("driver", "user", "password", "database", "host", "port");
    Handle h (driver, getlogin(), "", "dbicpp");

    // Set trace on and log queries to stderr
    trace(true, 2);

    // create table.
    cout << endl;
    cout << "-- creating table --" << endl;

    // create test table
    h.execute("drop table if exists users");
    h.execute("create table users(id serial, name text, email text)");

    // create table.
    cout << endl;
    cout << "-- inserting test data --" << endl;

    // insert some test data
    Statement tdata (h, "insert into users(name, email) values(?, ?)");
    tdata % "Apple Arthurton", "apple@example.com", execute();
    tdata % "Benny Arthurton", "benny@example.com", execute();

    // prepare, bind and execute in one go.
    ( h << "insert into users(name, email) values (?, ?)" ) % "James Arthurton", "james@example.com", execute();

    cout << endl;
    cout << "-- simple select --" << endl;

    Statement st (h, "select id, name, email from users where id >= ? and id < ?");

    /*if (driver == "postgresql") {
        cout << endl;
        cout << "-- trying to kill connection for an auto-reconnect --" << endl;
        system("sudo kill `pgrep -U postgres -f postgres.*dbicpp`");
        sleep(1);
    }*/

    // bind and execute the statement in one go.
    st % 1L, 10L, execute();
    printResultRows(st);
    st.finish();

    cout << endl;
    cout << "-- fetchRowHash --" << endl;

    // bind and execute the statement later.
    st % 1L, 10L;
    st.execute();

    ResultRowHash rh;
    while (rh = st.fetchRowHash()) {
        cout << rh["name"] << endl;
    }

    cout << endl;
    cout << "-- rewind --" << endl;

    st.rewind();
    while (rh = st.fetchRowHash()) {
        cout << rh["name"] << endl;
    }

    st.finish();

    // nested transaction
    cout << endl;
    cout << "-- nested transaction --" << endl;

    h.begin("transaction_1");
    cout << "-- deleted rows with id = 1 --" << endl;
    h.execute("delete from users where id = 1");

    h.begin("transaction_2");
    cout << "-- deleted rows with id = 2 --" << endl;
    h.execute("delete from users where id = 2");

    cout << "-- rollback last statement --" << endl;
    h.rollback("transaction_2");
    h.commit("transaction_1");

    // transaction
    cout << endl;
    cout << "-- inserts & selects inside transaction --" << endl;

    string sql("insert into users (name, email) values (?, ?)");
    Statement ins (h, sql + (driver == "postgresql" ? " RETURNING id" : ""));

    ins % "John Doe", "doe@example.com", execute();
    cout << "-- inserted 1 row, last insert id = " << ins.lastInsertID() << " --" << endl;

    ins % "Jane Doe", null(), execute();
    cout << "-- inserted 1 row with null email value --" << endl;

    // type specific binding.
    st << "select id, name, email from users where id > %d", 0L, execute();
    printResultRows(st);
    st.finish();

    cout << endl;
    cout << "-- bulk copy in --" << endl;
    ResultRow fields(2, "name", "email");
    IOStream buffer;
    buffer.write("sally\tsally@local\n");
    buffer.write("jonas\tjonas@local\n");
    cout << "rows: " << h.copyIn("users", fields, &buffer) << endl;

    st << "select id, name, email from users where id > %d", 0L, execute();
    printResultRows(st);
    st.finish();

    // async querying.
    cout << endl;
    cout << "-- asynchronous query --" << endl;

    string sleep_sql = (driver == "mysql" ? "select sleep" : "select pg_sleep");

    ConnectionPool pool(5, driver, getlogin(), "", "dbicpp");
    Reactor::initialize();
    Reactor::watch(pool.execute(sleep_sql + "(0.5), 1 AS id", callback));
    Reactor::watch(pool.execute(sleep_sql + "(0.3), 2 AS id", callback));
    Reactor::watch(pool.execute(sleep_sql + "(0.1), 3 AS id", callback));
    Reactor::run();

    // single shot queries.
    cout << endl;
    cout << "-- result sets --" << endl;
    h.execute("select * from users");
    AbstractResultSet *rs = h.results();
    callback(rs);
    delete rs;
    cout << endl;
}
