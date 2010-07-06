#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <libpq-fe.h>
#include <getopt.h>
#include <string>
#include <cstring>
#include <vector>
#include <pcrecpp.h>

#define PQVALUE(res, r, c) PQgetisnull(res, r, c) ? NULL : PQgetvalue(res, r, c)

using namespace std;
using namespace pcrecpp;

char sql[4096], driver[4096];
vector<string> bind;
long max_iter;

void parseOptions(int argc, char **argv) {
    int option_index, c;
    static struct option long_options[] = {
        { "sql",    required_argument, 0, 's' },
        { "bind",   required_argument, 0, 'b' },
        { "num",    required_argument, 0, 'n' },
        { "driver", required_argument, 0, 'd' }
    };

    strcpy(driver, "postgresql");

    while ((c = getopt_long(argc, argv, "s:b:n:d:", long_options, &option_index)) >= 0) {
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch(c) {
            case 's': strcpy(sql, optarg); break;
            case 'b': bind.push_back(optarg); break;
            case 'n': max_iter = atol(optarg); break;
            case 'd': strcpy(driver, optarg); break;
            default: exit(1);
        }
    }
}

void error (const char *msg) {
    printf("ERROR: %s\n", msg);
    exit(1);
}

void pgCheckResult(PGresult *result) {
    switch(PQresultStatus(result)) {
        case PGRES_TUPLES_OK:
        case PGRES_COPY_OUT:
        case PGRES_COPY_IN:
        case PGRES_EMPTY_QUERY:
        case PGRES_COMMAND_OK:
            return;
        case PGRES_BAD_RESPONSE:
        case PGRES_FATAL_ERROR:
        case PGRES_NONFATAL_ERROR:
            error(PQresultErrorMessage(result));
            break;
        default:
            error("Unknown error, check logs.");
    }
}

// TODO: raise parser errors here before sending it to server ?
void pgPreProcessQuery(string &query) {
    int i, n = 0;
    char repl[128];
    string var;
    RE re("(?<!')(\\?)(?!')");
    while (re.PartialMatch(query, &var)) {
        sprintf(repl, "$%d", ++n);
        re.Replace(repl, &query);
    }
}

int main(int argc, char*argv[]) {
    int i, n;
    parseOptions(argc, argv);
    PGconn *conn = PQconnectdb("host=127.0.0.1 user=udbicpp dbname=dbicpp");
    if (conn == NULL)
        error("Unable to allocate connection");
    else if (PQstatus(conn) == CONNECTION_BAD)
        error(PQerrorMessage(conn));

    int *bind_l = new int[bind.size()];
    char **bind_v = new char*[bind.size()];

    for (n = 0; n < bind.size(); n++) {
        bind_v[n] = (char *)bind[n].data();
        bind_l[n] = bind[n].length();
    }

    string query(sql);
    pgPreProcessQuery(query);
    strcpy(sql, query.c_str());
    PGresult *r = PQprepare(conn, "myquery", sql, 0, 0);
    pgCheckResult(r);

    for (n = 0; n < (int)max_iter; n++) {
        r = PQexecPrepared(conn, "myquery", bind.size(), (const char* const *)bind_v, (const int *)bind_l, 0, 0);
        pgCheckResult(r);
        for (i = 0; i < PQntuples(r); i++)
            printf("%s %s %s\n", PQVALUE(r, i, 0), PQVALUE(r, i, 1), PQVALUE(r, i, 2));
        PQclear(r);
    }

    delete []bind_l;
    delete []bind_v;

    PQfinish(conn);
}