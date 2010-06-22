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
    ResultRow r;

    for (int n = 0; n < max; n++) {
        st.execute();
        while ((r = st.fetchRow()).size() > 0)
            printf("%s\t%s\t%s\n", r[0].value.c_str(), r[1].value.c_str(), r[2].value.c_str());
        st.finish();
    }
}
