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
        delete h;
    }

    uint32_t Handle::execute(string sql) {
        if (_trace) logMessage(_trace_fd, sql);
        return h->execute(sql);
    }

    uint32_t Handle::execute(string sql, vector<Param> &bind) {
        if (_trace) logMessage(_trace_fd, sql);
        return h->execute(sql, bind);
    }

    Result* Handle::result() {
        return new Result(h->result());
    }

    Statement* Handle::prepare(string sql) {
        return new Statement(h->prepare(sql));
    }

    // syntactic sugar.
    Statement* Handle::operator<<(string sql) {
        return new Statement(h->prepare(sql));
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

    uint64_t Handle::write(string table, FieldSet &fields, IO* io) {
        return h->write(table, fields, io);
    }

    void Handle::setTimeZoneOffset(int tzhour, int tzmin) {
        h->setTimeZoneOffset(tzhour, tzmin);
    }

    void Handle::setTimeZone(char *name) {
        h->setTimeZone(name);
    }

    string Handle::escape(string value) {
        return h->escape(value);
    }

    string Handle::driver() {
        return h->driver();
    }
}
