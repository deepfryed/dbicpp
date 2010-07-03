#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

void error (const char *msg) {
    printf("ERROR: %s\n", msg);
    exit(1);
}

int main(int argc, char*argv[]) {
    int i, n, max;
    max = argc == 2 ? atoi(argv[1]) : 10000;
    PGconn *conn = PQconnectdb("host=127.0.0.1 user=udbicpp dbname=dbicpp");
    if (conn == NULL)
        error("Unable to allocate connection");
    else if (PQstatus(conn) == CONNECTION_BAD)
        error(PQerrorMessage(conn));

    int bind_l[2] = { 1, 1 };
    const char *bind_v[2] = { "1", "2" };

    PGresult *r = PQprepare(conn, "myquery", "SELECT id, name, email FROM users WHERE id IN ($1, $2)", 0, 0);

    for (n = 0; n < max; n++) {
        r = PQexecPrepared(conn, "myquery", 2, (const char* const *)bind_v, (const int *)bind_l, 0, 0);
        for (i = 0; i < PQntuples(r); i++)
            printf("%s\t%s\t%s\n", PQgetvalue(r, i, 0), PQgetvalue(r, i, 1), PQgetvalue(r, i, 2));
        PQclear(r);
    }

    PQfinish(conn);
}
