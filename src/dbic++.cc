#include "dbic++.h"
#include <sstream>
#include <cstdio>

#define CONNECT_FUNC(f) ((AbstractHandle* (*)(string, string, string, string, string)) f)

namespace dbi {

    map<string, Driver *> drivers;
    vector<string> available_drivers();

    string generateCompactUUID() {
        string rv;
        char hex[3];
        unsigned char uuid[16];

        uuid_generate(uuid);

        for (int i = 0; i < 16; i++) {
            sprintf(hex, "%02X", uuid[i]);
            rv += hex;
        }

        return rv;
    }

    bool dbiInitialize(string path = DEFAULT_DRIVER_PATH) {
        string filename;
        struct dirent *file;
        Driver* (*info)(void);
        pcrecpp::RE re("\\.so\\.\\d+");

        _trace          = false;
        _trace_fd       = 1;
        drivers["null"] = NULL;

        DIR *dir = opendir(path.c_str());
        if (!dir)
            return false;

        while((file = readdir(dir))) {
            if (file->d_type != DT_REG)
                continue;
            if (!re.PartialMatch(file->d_name))
                continue;

            filename = path + "/" + string(file->d_name);
            void *handle = dlopen(filename.c_str(), RTLD_NOW|RTLD_LOCAL);

            if (handle != NULL) {
                if ((info = (Driver* (*)(void)) dlsym(handle, "dbdInfo"))) {
                    Driver *driver = info();
                    driver->handle = handle;
                    driver->connect = CONNECT_FUNC(dlsym(handle, "dbdConnect"));

                    if (driver->connect == NULL)
                        throw InvalidDriverError(dlerror());

                    if (drivers[driver->name]) {
                        cout << "[WARNING] Already initialized " << driver->name << " driver."
                             << " Ignoring: " << filename << endl;
                        dlclose(handle);
                        delete driver;
                    }
                    else {
                        drivers[driver->name] = driver;
                    }
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
            if (driver)
                delete driver;
        }
    }

    vector<string> available_drivers () {
        vector <string>list;

        if (!drivers.size())
            dbiInitialize();
        for(map<string, Driver *>::iterator iter = drivers.begin(); iter != drivers.end(); ++iter) {
            list.push_back(iter->first);
        }

        return list;
    }

    void init_and_check(string driver_name) {
        if (!drivers.size()) {
            dbiInitialize("./lib/dbic++");
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


    Handle::Handle(AbstractHandle *ah)                       { h = ah; }
    Handle::~Handle()                                        { close(); h->cleanup(); delete h; }
    Statement Handle::prepare(string sql)                    { return Statement(h->prepare(sql)); }
    bool Handle::begin()                                     { return h->begin(); }
    bool Handle::commit()                                    { trx.clear(); return h->commit(); }
    bool Handle::rollback()                                  { trx.clear(); return h->rollback(); }
    bool Handle::begin(string name)                          { trx.push_back(name); return h->begin(name); }
    bool Handle::commit(string name)                         { trx.pop_back(); return h->commit(name); }
    bool Handle::rollback(string name)                       { trx.pop_back(); return h->rollback(name); }
    bool Handle::close()                                     { return h->close(); }
    void* Handle::call(string name, void* arg)               { return h->call(name, arg); }
    vector<string>& Handle::transactions()                   { return trx; }

    Statement::Statement()                                   { st = NULL; h = NULL; }
    Statement::Statement(AbstractStatement *ast)             { st = ast; }
    Statement::Statement(Handle &handle)                     { st = NULL; h = handle.h; }
    Statement::Statement(Handle &handle, string sql)         { h = handle.h; st = h->prepare(sql); }
    Statement::Statement(Handle *handle)                     { st = NULL; h = handle->h; }
    Statement::Statement(Handle *handle, string sql)         { h = handle->h; st = h->prepare(sql); }
    Statement::~Statement()                                  { finish(); if (st != NULL) { st->cleanup(); delete st; } }
    unsigned int Statement::rows()                           { return st->rows(); }
    ResultRow& Statement::fetchRow()                         { return st->fetchRow(); }
    ResultRowHash& Statement::fetchRowHash()                 { return st->fetchRowHash(); }
    bool Statement::finish()                                 { params.clear(); return st->finish(); }


    // syntactic sugar.
    Statement Handle::operator<<(string sql)                 { return Statement(h->prepare(sql)); }
    Statement& Statement::operator,(string v)                { bind(PARAM(v)); return *this; }
    Statement& Statement::operator%(string v)                { bind(PARAM(v)); return *this; }
    Statement& Statement::operator,(long v)                  { bind(v); return *this; }
    Statement& Statement::operator%(long v)                  { bind(v); return *this; }
    Statement& Statement::operator,(double v)                { bind(v); return *this; }
    Statement& Statement::operator%(double v)                { bind(v); return *this; }
    Statement& Statement::operator,(dbi::null const &e)      { bind(PARAM(e)); return *this; }
    Statement& Statement::operator%(dbi::null const &e)      { bind(PARAM(e)); return *this; }

    Statement& Statement::operator<<(string sql) {
        params.clear();

        if (st)
            delete st;
        if (!h)
            throw RuntimeError("Unable to call prepare() without database handle.");

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

    unsigned char* Statement::fetchValue(unsigned int r, unsigned int c, unsigned long *l = 0) {
        return st->fetchValue(r, c, l);
    }

    unsigned char* Statement::operator()(unsigned int r, unsigned int c) {
        return st->fetchValue(r, c, 0);
    }

    unsigned int Statement::currentRow() {
        return st->currentRow();
    }

    void Statement::advanceRow() {
        st->advanceRow();
    }

    bool trace() {
        return _trace;
    }

    void trace(bool flag) {
        _trace = flag;
    }

    void trace(bool flag, int fd)  {
        _trace = flag;
        _trace_fd = fd;
    }

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
        unsigned int rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    unsigned int Statement::execute(ResultRow &bind) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));
        return st->execute(bind);
    }

    unsigned int Statement::operator,(dbi::execute const &e) {
        unsigned int rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    unsigned long Statement::lastInsertID() {
        return st->lastInsertID();
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


    ConnectionPool::ConnectionPool(
        int size, string driver_name, string user, string pass, string dbname, string host, string port
    ) {
        init_and_check(driver_name);
        struct Connection c;
        for (int n = 0; n < size; n++) {
            AbstractHandle *h = drivers[driver_name]->connect(user, pass, dbname, host, port);
            h->initAsync();
            c.handle = h; c.busy = false;
            pool.push_back(c);
        }
    }

    ConnectionPool::ConnectionPool (int size, string driver_name, string user, string pass, string dbname) {
        init_and_check(driver_name);
        struct Connection c;
        for (int n = 0; n < size; n++) {
            AbstractHandle *h = drivers[driver_name]->connect(user, pass, dbname, "", "");
            h->initAsync();
            c.handle = h; c.busy = false;
            pool.push_back(c);
        }
    }

    bool ConnectionPool::execute(string sql, vector<Param> &bind, void (*callback)(AbstractResultSet *r)) {
        for (int n = 0; n < pool.size(); n++) {
            if (!pool[n].busy) {

                AbstractHandle *h = pool[n].handle;
                AbstractResultSet *rs = h->aexecute(sql, bind);

                struct event *ev = new struct event;
                struct Query *q  = new struct Query;
                *q = { this, h, rs, ev, callback };

                event_set(ev, h->socket(), EV_READ | EV_PERSIST, eventCallback, q);
                event_add(ev, 0);
                pool[n].busy = true;
                return true;
            }
        }
        return false;
    }

    bool ConnectionPool::execute(string sql, void (*callback)(AbstractResultSet *r)) {
        vector<Param> bind;
        return execute(sql, bind, callback);
    }

    ConnectionPool::~ConnectionPool() {
        for (int n = 0; n < pool.size(); n++) {
            pool[n].handle->close();
            pool[n].handle->cleanup();
            delete pool[n].handle;
        }
    }

    void ConnectionPool::process(struct Query *q) {
        AbstractResultSet *rs = q->result;
        void (*callback)(AbstractResultSet *r) = q->callback;
        if(!rs->consumeResult()) {
            rs->prepareResult();
            event_del(q->ev);
            for (int n = 0; n < pool.size(); n++) {
                if (q->handle == pool[n].handle) {
                    pool[n].busy = false;
                    break;
                }
            }
            delete q->ev;
            delete q;
            callback(rs);
        }
    }

    void ConnectionPool::eventCallback(int fd, short type, void *arg) {
        ((struct Query*)arg)->target->process((struct Query*)arg);
    }
}
