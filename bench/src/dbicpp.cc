#include "dbic++.h"
#include <cstdio>
#include <getopt.h>
#include <unistd.h>

using namespace std;
using namespace dbi;

long max_iter = 1;
FILE *outfile = stdout;
vector<Param> values;
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

    strcpy(driver, "postgresql");

    while ((c = getopt_long(argc, argv, "s:b:n:d:o:", long_options, &option_index)) >= 0) {
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch(c) {
            case 's': strcpy(sql, optarg); break;
            case 'b': values.push_back(PARAM(optarg)); break;
            case 'n': max_iter = atol(optarg); break;
            case 'd': strcpy(driver, optarg); break;
            case 'o': outfile = fopen(optarg, "w");
                      if (!outfile) { perror("Unabled to open file"); exit(1); }
                      break;
            default:  exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    int n, r, c;
    parseOptions(argc, argv);

    Handle h(driver, getlogin(), "", "dbicpp");
    Statement st(h, sql);

    for (n = 0; n < max_iter; n++) {
        st.execute(values);
        Result *res = st.result();
        for (r = 0; r < res->rows(); r++) {
            for (c = 0; c < res->columns(); c++)
                fprintf(outfile, "%s\t", res->read(r,c,0));
            fprintf(outfile, "\n");
        }
        delete res;
    }
}
