#include "dbic++.h"

namespace dbi {
    extern map<string, Driver *> drivers;
    void initCheck(string);

    Handle::Handle(string driver_name, string user, string pass, string dbname, string host, string port) {
        initCheck(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, host, port);
    }

    Handle::Handle(string driver_name, string user, string pass, string dbname) {
        initCheck(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, "", "");
    }

    AbstractHandle* Handle::conn() {
        return h;
    }

    Handle::Handle(AbstractHandle *ah) {
        h = ah;
    }

    Handle::~Handle() {
        h->close();
        h->cleanup();
        delete h;
    }

    Statement Handle::prepare(string sql) {
        return Statement(h->prepare(sql));
    }

    bool Handle::begin() {
        return h->begin();
    }

    bool Handle::commit() {
        trx.clear();
        return h->commit();
    }

    bool Handle::rollback() {
        trx.clear();
        return h->rollback();
    }

    bool Handle::begin(string name) {
        trx.push_back(name);
        return h->begin(name);
    }

    bool Handle::commit(string name) {
        trx.pop_back();
        return h->commit(name);
    }

    bool Handle::rollback(string name) {
        trx.pop_back();
        return h->rollback(name);
    }

    bool Handle::close() {
        return h->close();
    }

    vector<string>& Handle::transactions() {
        return trx;
    }

    void* Handle::call(string name, void* arg, unsigned long l) {
        return h->call(name, arg, l);
    }
}