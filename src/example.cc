#include "dbic++.h"

/*----------------------------------------------------------------------------------

   To compile:

   g++ -Iinc -Llibs -rdynamic -o example example.cc -ldbic++ -ldl -lpcrecpp -levent

----------------------------------------------------------------------------------*/

using namespace std;
using namespace dbi;

void printResultRows(Statement &st) {
    ResultRow r;
    while ((r = st.fetchRow()).size() > 0) {
        cout << r.join("\t") << endl;
    }
}

void callback(AbstractResultSet *r) {
    for (int i = 0; i < r->rows(); i++) {
        cout << r->fetchRowHash() << endl;
    }
    r->cleanup();
    delete r;
}

int main(int argc, char *argv[]) {
    string driver(argc > 1 ? argv[1] : "postgresql");

    // Handle h ("driver", "user", "password", "database", "host", "port");
    Handle h (driver, "udbicpp", "", "dbicpp");

    // Set trace on and log queries to stderr
    // trace(true, 2);

    // create table.
    cout << endl;
    cout << "Creating table" << endl;
    cout << "--------------" << endl << endl;

    // create test table
    h.execute("DROP TABLE IF EXISTS users");
    h.execute("CREATE TABLE users(id serial, name text, email text)");

    // create table.
    cout << endl;
    cout << "Inserting test data" << endl;
    cout << "-------------------" << endl << endl;

    // insert some test data
    Statement tdata (h, "INSERT INTO users(name, email) VALUES(?, ?)");
    tdata % "Apple Arthurton", "apple@example.com", execute();
    tdata % "Benny Arthurton", "benny@example.com", execute();

    // prepare, bind and execute in one go.
    ( h << "INSERT INTO users(name, email) VALUES (?, ?)" ) % "James Arthurton", "james@example.com", execute();

    cout << "Simple select" << endl;
    cout << "-------------" << endl << endl;

    Statement st (h, "SELECT id, name, email FROM users WHERE id >= ? AND id < ?");

    // bind and execute the statement in one go.
    st % 1L, 10L, execute();
    printResultRows(st);
    st.finish();

    cout << endl;
    cout << "fetchRowHash" << endl;
    cout << "------------" << endl << endl;

    // bind and execute the statement later.
    st % 1L, 10L;
    st.execute();
    ResultRowHash rh;
    while ((rh = st.fetchRowHash()).size() > 0) {
        cout << rh["name"] << endl;
    }
    st.finish();

    // nested transaction
    cout << endl;
    cout << "Nested transaction" << endl;
    cout << "------------------" << endl << endl;

    h.begin("transaction_1");
    cout << "deleted rows with id = 1: " << h.execute("DELETE FROM users WHERE id = 1") << endl;
    h.begin("transaction_2");
    cout << "deleted rows with id = 2: " << h.execute("DELETE FROM users WHERE id = 2") << endl;
    cout << "rollback last statement" << endl;
    h.rollback("transaction_2");
    h.commit("transaction_1");

    // transaction
    cout << endl;
    cout << "Inserts & Selects inside transaction" << endl;
    cout << "------------------------------------" << endl << endl;

    string sql("INSERT INTO users (name, email) VALUES (?, ?)");
    Statement ins (h, sql + (driver == "postgresql" ? " RETURNING id" : ""));
    ins % "John Doe", "doe@example.com", execute();
    cout << "Inserted 1 row, last insert id = " << ins.lastInsertID() << endl;
    ins % "Jane Doe", null(), execute();
    cout << "Inserted 1 row with null email value" << endl << endl;

    // type specific binding.
    st << "SELECT id, name, email FROM users WHERE id > %d", 0L, execute();
    printResultRows(st);
    st.finish();

    // async querying.
    cout << endl;
    cout << "Asynchronous query" << endl;
    cout << "------------------" << endl << endl;

    string sleep_sql = (driver == "mysql" ? "SELECT sleep" : "SELECT pg_sleep");

    event_init();
    ConnectionPool pool(5, driver, "udbicpp", "", "dbicpp");
    pool.execute(sleep_sql + "(0.5), 1 AS id", callback);
    pool.execute(sleep_sql + "(0.3), 2 AS id", callback);
    pool.execute(sleep_sql + "(0.1), 3 AS id", callback);
    event_loop(0);

    cout << endl;
}
