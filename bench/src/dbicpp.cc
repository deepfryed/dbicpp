#include "dbic++.h"
#include <cstdio>
#include <getopt.h>

using namespace std;
using namespace dbi;

char sql[4096], driver[4096];
ResultRow bind;
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
            case 'b': bind << string(optarg); break;
            case 'n': max_iter = atol(optarg); break;
            case 'd': strcpy(driver, optarg); break;
            default: exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    int n, r, c, rows, cols;
    dbiInitialize("../lib/dbic++");
    parseOptions(argc, argv);

    Handle h(driver, "udbicpp", "", "dbicpp");
    Statement st(h, sql);
    ResultRow row;

    for (n = 0; n < max_iter; n++) {
        rows = (int) st.execute(bind);
        cols = (int) st.columns();
        while ((row = st.fetchRow()).size() > 0) {
            for (c = 0; c < row.size(); c++)
                printf("%s\t", row[c].value.c_str());
            printf("\n");
        }

        // Speed: fetchValue() > fetchRow() > fetchRowHash()
        /*for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                printf("%s\t", st.fetchValue(r, c , 0));
            }
            printf("\n");
        }*/
        st.finish();
    }
}
