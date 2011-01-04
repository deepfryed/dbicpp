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
        uint32_t cols = sqlite3_bind_parameter_count(stmt);
        if (bind.size() != cols) {
            snprintf(errormsg, 8192, "Given %d but expected %d bind args", (int)bind.size(), cols);
            throw RuntimeError(errormsg);
        }

        for (uint32_t i = 0; i < cols; i++) {
            if (bind[i].isnull) {
                sqlite3_bind_null(stmt, i+1);
                continue;
            }
            sqlite3_bind_text(stmt, i+1, bind[i].value.c_str(), bind[i].value.length(), 0);
        }
    }
}

using namespace std;
using namespace dbi;

extern "C" {
    Sqlite3Handle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        return new Sqlite3Handle(dbname);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
