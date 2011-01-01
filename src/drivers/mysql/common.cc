#include "common.h"

namespace dbi {

    char errormsg[8192];

    char MYSQL_BOOL_TRUE  = 1;
    char MYSQL_BOOL_FALSE = 0;

    map<string, IO*> CopyInList;

    // MYSQL does not support type specific binding
    void MYSQL_PREPROCESS_QUERY(string &query) {
        string var;

        RE re("(?<!')(%(?:[dsfu]|l[dfu]))(?!')");
        while (re.PartialMatch(query, &var))
            re.Replace("?", &query);
    }

    void MYSQL_INTERPOLATE_BIND(MYSQL *conn, string &query, vector<Param> &bind) {
        int n = 0;
        unsigned long len, repl_len = 1024;
        char *repl = new char[repl_len];
        string orig = query, var;

        RE re("(?<!')(\\?)(?!')");
        while (re.PartialMatch(query, &var)) {
            if (n >= bind.size()) {
                if (repl) delete [] repl;
                sprintf(errormsg, "Only %d parameters provided for query: %s", (int)bind.size(), orig.c_str());
                throw RuntimeError(errormsg);
            }
            if (bind[n].isnull) {
                len  = 4;
                strcpy(repl, "NULL");
            }
            else {
                len = bind[n].value.length()*2 + 3;
                if (len > repl_len) {
                    repl_len = len + 2;
                    delete [] repl;
                    repl = new char[repl_len];
                }
                len  = mysql_real_escape_string(conn, repl+1, bind[n].value.c_str(), bind[n].value.length());
                repl[0] = '\'';
                repl[len + 1] = '\'';
                repl[len + 2] = 0;
                len += 2;
            }
            re.Replace(string(repl, len), &query);
            n++;
        }

        if (repl) delete [] repl;
    }

    bool MYSQL_CONNECTION_ERROR(int error) {
        return (error == CR_SERVER_GONE_ERROR || error == CR_SERVER_LOST ||
            error == CR_SERVER_LOST_EXTENDED || error == CR_COMMANDS_OUT_OF_SYNC);
    }


    int LOCAL_INFILE_INIT(void **ptr, const char *filename, void *unused) {
        *ptr = (void*)CopyInList[filename];
        if (*ptr)
            return 0;
        else
            return -1;
    }

    int LOCAL_INFILE_READ(void *ptr, char *buffer, uint32_t len) {
        len = ((IO*)ptr)->read(buffer, len);
        return len;
    }

    void LOCAL_INFILE_END(void *ptr) {
        for (map<string, IO*>::iterator it = CopyInList.begin(); it != CopyInList.end(); it++) {
            if (it->second == (IO*)ptr) {
                CopyInList.erase(it);
                return;
            }
        }
    }

    int LOCAL_INFILE_ERROR(void *ptr, char *error, uint32_t len) {
        strcpy(error, ptr ? "Unknown error while bulk loading data" : "Unable to find resource to copy data.");
        return CR_UNKNOWN_ERROR;
    }
}

using namespace std;
using namespace dbi;

extern "C" {
    MySqlHandle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        if (host == "")
            host = "127.0.0.1";
        if (port == "0" || port == "")
            port = "3306";

        return new MySqlHandle(user, pass, dbname, host, port);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
