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
        vector<string> parts;
        const char *cptr = query.c_str();
        uint64_t max = query.length(),  begin = 0, end = 0;
        while (begin < max) {
            while (end < max && *(cptr+end) != '?') end++;
            parts.push_back(string(cptr+begin, end-begin));
            begin = ++end;
        }

        query = "";
        uint64_t len, alloc = 1024;
        char *escaped = new char[alloc];
        for (int n = 0; n < parts.size(); n++) {
            query += parts[n];
            if (n < bind.size()) {
                if (bind[n].isnull) {
                    query += "NULL";
                }
                else {
                    len = bind[n].value.length()*2 + 3;
                    if (len > alloc) {
                        alloc = len + 2;
                        delete [] escaped;
                        escaped = new char[alloc];
                    }

                    len = mysql_real_escape_string(conn, escaped+1, bind[n].value.c_str(), bind[n].value.length());
                    escaped[0] = '\'';
                    escaped[len + 1] = '\'';
                    escaped[len + 2] = 0;
                    len += 2;

                    query += string(escaped, len);
                }
            }
        }

        if (escaped) delete [] escaped;
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
