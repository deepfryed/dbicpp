#include <mysql++/mysql++.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>

using namespace std;

long max_iter = 1;
FILE *outfile = stdout;
vector<string> bind;
char sql[4096], driver[4096];

void parseOptions(int argc, char **argv) {
    int option_index, c;
    static struct option long_options[] = {
        { "sql",    required_argument, 0, 's' },
        { "bind",   required_argument, 0, 'b' },
        { "num",    required_argument, 0, 'n' },
        { "driver", required_argument, 0, 'd' },
        { "output", optional_argument, 0, 'o' }
    };

    max_iter = 1;
    strcpy(driver, "mysql");

    while ((c = getopt_long(argc, argv, "s:b:n:d:o:", long_options, &option_index)) >= 0) {
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch(c) {
            case 's': strcpy(sql, optarg); break;
            case 'b': bind.push_back(optarg); break;
            case 'n': max_iter = atol(optarg); break;
            case 'd': strcpy(driver, optarg); break;
            case 'o': outfile = fopen(optarg, "w");
                      if (!outfile) { perror("Unabled to open file"); exit(1); }
                      break;
            default: exit(1);
        }
    }
}

void error (const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Connect to the sample database.
    parseOptions(argc, argv);
    mysqlpp::Connection conn(false);
    if (conn.connect("dbicpp", "127.0.0.1", getlogin(), "")) {
        for (int n = 0; n < max_iter; n++) {
            mysqlpp::Query query = conn.query(sql);
            if (mysqlpp::StoreQueryResult res = query.store()) {
                for (size_t i = 0; i < res.num_rows(); ++i) {
                    for (size_t j = 0; j < res.num_fields(); ++j) {
                        fprintf(outfile, "%s\t", res[i][j].c_str());
                    }
                    fprintf(outfile, "\n");
                }
            }
            else {
                cerr << "Failed to get item list: " << query.error() << endl;
                return 1;
            }
        }
    }
    else {
        cerr << "DB connection failed: " << conn.error() << endl;
    }
}
