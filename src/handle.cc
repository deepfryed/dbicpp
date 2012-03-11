#include "dbic++.h"

namespace dbi {
    extern map<string, Driver *> drivers;
    void initCheck(string);

    Handle::Handle(string driver_name, string user, string pass, string dbname, string host, string port, char *options) {
        initCheck(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, host, port, options);
    }

    Handle::Handle(string driver_name, string user, string pass, string dbname) {
        initCheck(driver_name);
        h = drivers[driver_name]->connect(user, pass, dbname, "", "", 0);
    }

    AbstractHandle* Handle::conn() {
        return h;
    }

    Handle::Handle(AbstractHandle *ah) {
        h = ah;
    }

    Handle::~Handle() {
        if (h) delete h;
        h = 0;
    }

    uint32_t Handle::execute(string sql) {
        if (_trace) logMessage(_trace_fd, sql);
        return h->execute(sql);
    }

    uint32_t Handle::execute(string sql, param_list_t &bind) {
        if (_trace) logMessage(_trace_fd, sql);
        return h->execute(sql, bind);
    }

    Result* Handle::aexecute(string sql) {
        if (_trace) logMessage(_trace_fd, sql);
        return new Result(h->aexecute(sql));
    }

    Result* Handle::aexecute(string sql, param_list_t &bind) {
        if (_trace) logMessage(_trace_fd, sql);
        return new Result(h->aexecute(sql, bind));
    }

    int Handle::socket() {
        return h->socket();
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
        if (_trace) logMessage(_trace_fd, "begin");
        return h->begin();
    }

    bool Handle::commit() {
        trx.clear();
        if (_trace) logMessage(_trace_fd, "commit");
        return h->commit();
    }

    bool Handle::rollback() {
        trx.clear();
        if (_trace) logMessage(_trace_fd, "rollback");
        return h->rollback();
    }

    bool Handle::begin(string name) {
        trx.push_back(name);
        if (_trace) logMessage(_trace_fd, "begin " + name);
        return h->begin(name);
    }

    bool Handle::commit(string name) {
        trx.pop_back();
        if (_trace) logMessage(_trace_fd, "commit " + name);
        return h->commit(name);
    }

    bool Handle::rollback(string name) {
        trx.pop_back();
        if (_trace) logMessage(_trace_fd, "rollback " + name);
        return h->rollback(name);
    }

    bool Handle::close() {
        return h->close();
    }

    string_list_t& Handle::transactions() {
        return trx;
    }

    uint64_t Handle::write(string table, field_list_t &fields, IO* io) {
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

    void Handle::reconnect() {
        h->reconnect();
    }
}
