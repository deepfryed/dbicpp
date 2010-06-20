#include "dbic++.h"
#include <sstream>
#include <cstdio>

#define CONNECT_FUNC(f) ((AbstractHandle* (*)(string, string, string, string, string)) f)

namespace dbi {

    map<string, Driver *> drivers;
    vector<string> available_drivers();

    bool dbiInitialize(string path = DEFAULT_DRIVER_PATH) {
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
        if (!drivers["null"]) dbiInitialize();
        for(map<string, Driver *>::iterator iter = drivers.begin(); iter != drivers.end(); ++iter) {
            list.push_back(iter->first);
        }
        return list;
    }

    Handle dbiConnect(string driver_name, string user, string pass, string dbname, string host, string port) {
        if (!drivers["null"]) { 
            dbiInitialize("./libs");
            dbiInitialize();
        }
        if (!drivers[driver_name])
            throw InvalidDriverError("Unable to find the '" + driver_name + "' driver.");
        return Handle(drivers[driver_name]->connect(user, pass, dbname, host, port));
    }

    Handle dbiConnect(string driver_name, string user, string pass, string dbname, string host) {
        return dbiConnect(driver_name, user, pass, dbname, host, "0");
    }

    Handle dbiConnect(string driver_name, string user, string pass, string dbname) {
        return dbiConnect(driver_name, user, pass, dbname, "127.0.0.1", "0");
    }

    Handle::Handle(AbstractHandle *ah)                                      { h = ah; }
    Handle::~Handle()                                                       { close(); delete h; }
    unsigned int Handle::execute(string sql)                                { return h->execute(sql); }
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
    Statement::~Statement()                                                 { finish(); if (st != NULL) delete st; }
    unsigned int Statement::rows()                                          { return st->rows(); }
    unsigned int Statement::execute()                                       { return st->execute(params); }
    unsigned int Statement::execute(vector<Param> &bind)                    { return st->execute(bind); }        
    ResultRow Statement::fetchRow()                                         { return st->fetchRow(); }
    ResultRowHash Statement::fetchRowHash()                                 { return st->fetchRowHash(); }
    bool Statement::finish()                                                { params.clear(); return st->finish(); }


    // syntactic sugar.
    Statement Handle::operator<<(string sql)                                { return Statement(h->prepare(sql)); }
    Statement& Statement::operator,(string v)                               { bind(Param(v)); return *this; }
    Statement& Statement::operator%(string v)                               { bind(Param(v)); return *this; }
    Statement& Statement::operator,(long   v)                               { bind(v); return *this; }
    Statement& Statement::operator%(long   v)                               { bind(v); return *this; }
    Statement& Statement::operator,(double v)                               { bind(v); return *this; }
    Statement& Statement::operator%(double v)                               { bind(v); return *this; }
    Statement& Statement::operator,(dbi::null const &e)                     { bind(Param(e)); return *this; }
    Statement& Statement::operator%(dbi::null const &e)                     { bind(Param(e)); return *this; }
    unsigned int Statement::operator,(dbi::execute const &e)                { return st->execute(params); }

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
        params.push_back(val);
    }
    void Statement::bind(double v) {
        char val[256];
        sprintf(val, "%lf", v);
        params.push_back(val);
    }
}
