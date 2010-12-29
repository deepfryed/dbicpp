#include "common.h"

namespace dbi {

    char errormsg[8192];

    char MYSQL_BOOL_TRUE  = 1;
    char MYSQL_BOOL_FALSE = 0;

    bool MYSQL_BIND_RO    = true;
    bool MYSQL_BIND_RW    = false;

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
        unsigned long len;
        char *repl = 0;
        string orig = query, var;

        RE re("(?<!')(\\?)(?!')");
        while (re.PartialMatch(query, &var)) {
            if (n >= bind.size()) {
                if (repl) free(repl);
                sprintf(errormsg, "Only %d parameters provided for query: %s", (int)bind.size(), orig.c_str());
                throw RuntimeError(errormsg);
            }
            if (bind[n].isnull) {
                repl = (char *)malloc(5);
                strcpy(repl, "NULL");
            }
            else {
                repl = (char*)malloc(bind[n].value.length()*2 + 3);
                len  = mysql_real_escape_string(conn, repl+1, bind[n].value.c_str(), bind[n].value.length());
                repl[0] = '\'';
                repl[len + 1] = '\'';
                repl[len + 2] = 0;
            }
            re.Replace(repl, &query);
            free(repl);
            repl = 0;
        }
    }

    void MYSQL_PROCESS_BIND(MYSQL_BIND *params, vector<Param>&bind) {
        for (int i = 0; i < bind.size(); i++) {
            params[i].buffer        = (void *)bind[i].value.data();
            params[i].buffer_length = bind[i].value.length();
            params[i].is_null       = bind[i].isnull ? &MYSQL_BOOL_TRUE : &MYSQL_BOOL_FALSE;
            params[i].buffer_type   = bind[i].isnull ? MYSQL_TYPE_NULL : MYSQL_TYPE_STRING;
        }
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

    // ----------------
    // MySqlBind
    // ----------------

    MySqlBind::MySqlBind(int n, bool readonly) {
        columns   = n;
        _readonly = readonly;
        allocate();
    }

    MySqlBind::~MySqlBind() {
        deallocate();
    }

    void MySqlBind::reallocate(int c, uint64_t length) {
        if (!_readonly) {
            delete [] (unsigned char*)params[c].buffer;
            params[c].buffer  = (void*)new unsigned char[length];
            params[c].buffer_length = length;
        }
    }

    void MySqlBind::allocate() {
        params = new MYSQL_BIND[columns];
        bzero(params, sizeof(MYSQL_BIND)*columns);

        if (!_readonly) {
            for (int i = 0; i < columns; i++) {
                params[i].length  = new unsigned long;
                params[i].is_null = new char;
                params[i].buffer  = (void*)new unsigned char[__MYSQL_BIND_BUFFER_LEN];
                params[i].buffer_length = __MYSQL_BIND_BUFFER_LEN;
            }
        }
    }

    void MySqlBind::deallocate() {
        if (!_readonly) {
            for (int i = 0; i < columns; i++) {
                delete params[i].length;
                delete params[i].is_null;
                delete [] (unsigned char*)params[i].buffer;
            }
        };

        delete [] params;
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
