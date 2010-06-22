#include "dbic++.h"
#include <sstream>
#include <cstdio>

#define CONNECT_FUNC(f) ((AbstractHandle* (*)(string, string, string, string, string)) f)

namespace dbi {

    map<string, Driver *> drivers;
    vector<string> available_drivers();

    bool dbiInitialize(string path = DEFAULT_DRIVER_PATH) {
        _trace    = false;
        _trace_fd = 1;
        pcrecpp::RE re("\\.so\\.\\d+");
        drivers["null"] = NULL;
        string filename;
        Driver* (*info)(void);
        DIR *dir = opendir(path.c_str());
        struct dirent *file;
        if (!dir) return false;
        while((file = readdir(dir))) {
            if (file->d_type != DT_REG) continue;
            if (!re.PartialMatch(file->d_name)) continue;
            filename = path + "/" + string(file->d_name);
            void *handle = dlopen(filename.c_str(), RTLD_NOW|RTLD_LOCAL);
            if (handle != NULL) {
                if ((info = (Driver* (*)(void)) dlsym(handle, "dbdInfo"))) {
                    Driver *driver = info();
                    driver->handle = handle;
                    driver->connect = CONNECT_FUNC(dlsym(handle, "dbdConnect"));
                    if (driver->connect == NULL) throw InvalidDriverError(dlerror());
                    drivers[driver->name] = driver;
                }
                else {
                    cout << "[WARNING] Ignoring " << filename << ":" << dlerror() << endl;
                }
            }
            else {
                cout << "[WARNING] Ignoring " << filename << ":" << dlerror() << endl;
            }
        }
        closedir(dir);
        return true;
    }

    void dbiShutdown() {
        for (map<string, Driver*>::iterator iter = drivers.begin(); iter != drivers.end(); ++iter) {
            Driver *driver = iter->second;
            if (driver) delete driver;
        }
    }

    vector<string> available_drivers () {
        vector <string>list;
        if (!drivers.size()) dbiInitialize();
        for(map<string, Driver *>::iterator iter = drivers.begin(); iter != drivers.end(); ++iter) {
            list.push_back(iter->first);
        }
        return list;
    }

    void init_and_check(string driver_name) {
        if (!drivers.size()) {
            dbiInitialize("./libs");
            dbiInitialize();
        }
        if (!drivers[driver_name])
            throw InvalidDriverError("Unable to find the '" + driver_name + "' driver.");
    }

    Handle::Handle(string driver_name, string user, string pass, string dbname, string host, string port) {
        init_and_check(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, host, port);
    }

    Handle::Handle(string driver_name, string user, string pass, string dbname) {
        init_and_check(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, "", "");
    }


    Handle::Handle(AbstractHandle *ah)                                      { h = ah; }
    Handle::~Handle()                                                       { close(); delete h; }
    Statement Handle::prepare(string sql)                                   { return Statement(h->prepare(sql)); }
    bool Handle::begin()                                                    { return h->begin(); }
    bool Handle::commit()                                                   { return h->commit(); }
    bool Handle::rollback()                                                 { return h->rollback(); }
    bool Handle::begin(string name)                                         { return h->begin(name); }
    bool Handle::commit(string name)                                        { return h->commit(name); }
    bool Handle::rollback(string name)                                      { return h->rollback(name); }
    bool Handle::close()                                                    { return h->close(); }
    void* Handle::call(string name, void* arg)                              { return h->call(name, arg); }

    Statement::Statement()                                                  { st = NULL; h = NULL; }
    Statement::Statement(AbstractStatement *ast)                            { st = ast; }
    Statement::Statement(Handle &handle)                                    { st = NULL; h = handle.h; }
    Statement::Statement(Handle &handle, string sql)                        { h = handle.h; st = h->prepare(sql); }
    Statement::Statement(Handle *handle)                                    { st = NULL; h = handle->h; }
    Statement::Statement(Handle *handle, string sql)                        { h = handle->h; st = h->prepare(sql); }
    Statement::~Statement()                                                 { finish(); if (st != NULL) delete st; }
    unsigned int Statement::rows()                                          { return st->rows(); }
    ResultRow Statement::fetchRow()                                         { return st->fetchRow(); }
    ResultRowHash Statement::fetchRowHash()                                 { return st->fetchRowHash(); }
    bool Statement::finish()                                                { params.clear(); return st->finish(); }


    // syntactic sugar.
    Statement Handle::operator<<(string sql)                                { return Statement(h->prepare(sql)); }
    Statement& Statement::operator,(string v)                               { bind(PARAM(v)); return *this; }
    Statement& Statement::operator%(string v)                               { bind(PARAM(v)); return *this; }
    Statement& Statement::operator,(long   v)                               { bind(v); return *this; }
    Statement& Statement::operator%(long   v)                               { bind(v); return *this; }
    Statement& Statement::operator,(double v)                               { bind(v); return *this; }
    Statement& Statement::operator%(double v)                               { bind(v); return *this; }
    Statement& Statement::operator,(dbi::null const &e)                     { bind(PARAM(e)); return *this; }
    Statement& Statement::operator%(dbi::null const &e)                     { bind(PARAM(e)); return *this; }

    Statement& Statement::operator<<(string sql) {
        params.clear();
        if (st) delete st;
        if (!h) throw RuntimeError("Unable to call prepare() without database handle.");
        st = h->prepare(sql);
        return *this;
    }

    void Statement::bind(Param v) {
        params.push_back(v);
    }

    void Statement::bind(long v) {
        char val[256];
        sprintf(val, "%ld", v);
        params.push_back(PARAM(val));
    }
    void Statement::bind(double v) {
        char val[256];
        sprintf(val, "%lf", v);
        params.push_back(PARAM(val));
    }

    vector<string> Statement::fields() {
        return st->fields();
    }

    unsigned int Statement::columns() {
        return st->columns();
    }

    unsigned char* Statement::fetchValue(int r, int c) { return st->fetchValue(r, c); }
    unsigned char* Statement::operator()(int r, int c) { return st->fetchValue(r, c); }

    unsigned int Statement::currentRow() { return st->currentRow(); }
    void Statement::advanceRow() { st->advanceRow(); }

    bool trace()                   { return _trace; }
    void trace(bool flag)          { _trace = flag; }
    void trace(bool flag, int fd)  { _trace = flag; _trace_fd = fd; }

    unsigned int Handle::execute(string sql) {
        if (_trace) logMessage(_trace_fd, sql);
        return h->execute(sql);
    }

    static string inline formatParams(string sql, ResultRow &p) {
        string message(sql);
        if (p.size() > 0) message += " : " + p.join(", ");
        return message;
    }

    unsigned int Statement::execute() {
        if (_trace) logMessage(_trace_fd, formatParams(st->command(), params));
        return st->execute(params);
    }

    unsigned int Statement::execute(ResultRow &bind) {
        if (_trace) logMessage(_trace_fd, formatParams(st->command(), bind));
        return st->execute(bind);
    }

    unsigned int Statement::operator,(dbi::execute const &e) {
        if (_trace) logMessage(_trace_fd, formatParams(st->command(), params));
        return st->execute(params);
    }

    void logMessage(int fd, string msg) {
        long n;
        char buffer[512];
        time_t now = time(NULL);
        struct tm *now_tm = localtime(&now);
        struct timeval tv;
        struct timezone tz;

        gettimeofday(&tv, &tz);
        strftime(buffer, 512, "[%FT%H:%M:%S", now_tm);
        n = write(fd, buffer, strlen(buffer));
        sprintf(buffer, ".%ld] ", tv.tv_usec);
        n += write(fd, buffer, strlen(buffer));
        n += write(fd, msg.data(), msg.length());
        n += write(fd, "\n", 1);
    }
}
