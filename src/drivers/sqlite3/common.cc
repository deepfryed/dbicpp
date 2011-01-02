#include "common.h"

namespace dbi {

    char errormsg[8192];

    void SQLITE3_PREPROCESS_QUERY(string &query) {
        string var;

        RE re("(?<!')(%(?:[dsfu]|l[dfu]))(?!')");
        while (re.PartialMatch(query, &var))
            re.Replace("?", &query);
    }

    void SQLITE3_PROCESS_BIND(sqlite3_stmt *stmt, vector<Param> &bind) {
        uint32_t cols = sqlite3_column_count(stmt);
        if (bind.size() != cols) {
            snprintf(errormsg, 8192, "Given %d but expected %d bind args", bind.size(), cols);
            throw RuntimeError(errormsg);
        }

        for (uint32_t i = 0; i < cols; i++) {
            bool isnull = bind[i].isnull;
            if (bind[i].isnull) {
                sqlite3_bind_null(stmt, i);
                continue;
            }
            sqlite3_bind_text(stmt, i, bind[i].value.c_str(), bind[i].value.length());
        }
    }

    void SQLITE3_INTERPOLATE_BIND(string &query, vector<Param> &bind) {
        int n = 0;
        char *repl;
        string orig = query, var;

        RE re("(?<!')(\\?)(?!')");
        while (re.PartialMatch(query, &var)) {
            if (n >= bind.size()) {
                if (repl) delete [] repl;
                sprintf(errormsg, "Only %d parameters provided for query: %s", (int)bind.size(), orig.c_str());
                throw RuntimeError(errormsg);
            }
            if (bind[n].isnull) {
                repl = sqlite3_mprintf("%Q", 0);
            }
            else {
                repl = sqlite3_mprintf("%Q", bind[n].value.c_str());
            }
            re.Replace(repl, &query);
            sqlite3_free(repl);
            n++;
        }
    }
}

using namespace std;
using namespace dbi;

extern "C" {
    Sqlite3Handle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        return Sqlite3Handle(dbname);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
