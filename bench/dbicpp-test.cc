#include "dbic++.h"
#include <cstdio>

using namespace std;
using namespace dbi;

int main(int argc, char *argv[]) {
    int max;
    dbiInitialize("../libs");
    max = argc == 2 ? atoi(argv[1]) : 10000;
    Handle h = dbiConnect("postgresql", "udbicpp", "", "dbicpp");

    Statement st(h, "SELECT id, name, email FROM users");

    for (int n = 0; n < max; n++) {
        st.execute();
        ResultRow r;
        while ((r = st.fetchRow()).size() > 0) {
            cout << r[0] << "\t" << r[1] << "\t" << r[2] << endl;
        }
        st.finish();
    }
}
