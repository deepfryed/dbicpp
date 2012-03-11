#include "dbic++.h"
#include <unistd.h>
#include <sys/select.h>

/*------------------------------------------------------------------------------

   To compile:

   g++ -Iinc -Llib -rdynamic -o async demo.cc -ldbic++ -ldl -lpcrecpp -luuid

------------------------------------------------------------------------------*/

using namespace std;
using namespace dbi;

ostream& operator<< (ostream &out, Result *res) {
    ResultRowHash row;

    while (res->read(row))
        out << row << endl;

    return out;
}


int main(int argc, char *argv[]) {
    // Handle::Handle(driver, user, password, database, host, port)
    Handle h1 ("postgresql", getlogin(), "", "dbicpp");
    Handle h2 ("postgresql", getlogin(), "", "dbicpp");
    Handle h3 ("postgresql", getlogin(), "", "dbicpp");

    // Set trace on and log queries to stderr
    trace(true, fileno(stderr));

    // create table.
    cout << endl;
    cout << "-- creating table --" << endl;

    // create test table
    h1.execute("drop table if exists users");
    h1.execute("create table users(id serial, name text, email text, primary key (id))");
    cout << endl;

    // insert some test data
    cout << "-- inserting test data --" << endl;

    Statement ins (h1, "insert into users(name, email) values(?, ?)");
    ins % "Apple Arthurton", "apple@example.com", execute();
    ins % "Benny Arthurton", "benny@example.com", execute();
    ins % "Marty Arthurton", null(), execute();
    cout << endl;

    cout << "-- async selects --" << endl;

    Result* res1 = h1.aexecute("select pg_sleep(3), 1 as query_id, count(*) from users");
    Result* res2 = h2.aexecute("select pg_sleep(2), 2 as query_id, count(*) from users");
    Result* res3 = h3.aexecute("select pg_sleep(1), 3 as query_id, count(*) from users");

    int done = 0;
    while (done < 3) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(h1.socket(), &fds);
        FD_SET(h2.socket(), &fds);
        FD_SET(h3.socket(), &fds);
        if (select(h3.socket() + 1, &fds, 0, 0, 0) < 0) {
            perror("select()");
            exit(1);
        }

        if (FD_ISSET(h1.socket(), &fds)) {
            res1->retrieve();
            cout << res1;
            done++;
        }

        if (FD_ISSET(h2.socket(), &fds)) {
            res2->retrieve();
            cout << res2;
            done++;
        }

        if (FD_ISSET(h3.socket(), &fds)) {
            res3->retrieve();
            cout << res3;
            done++;
        }
    }

    delete res1;
    delete res2;
    delete res3;
}
