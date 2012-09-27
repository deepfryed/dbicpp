#include "dbic++.h"

namespace dbi {

    Statement::Statement() {
        st = 0;
        h  = 0;
    }

    Statement::Statement(AbstractStatement *ast) {
        st = ast;
    }

    Statement::Statement(Handle &handle) {
        st = 0;
        h  = handle.h;
    }

    Statement::Statement(Handle &handle, string sql) {
        h  = handle.h;
        st = h->prepare(sql);
    }

    Statement::Statement(Handle *handle) {
        st = 0;
        h  = handle->h;
    }

    Statement::Statement(Handle *handle, string sql) {
        h  = handle->h;
        st = h->prepare(sql);
    }

    Statement::~Statement() {
        cleanup();
    }

    void Statement::finish() {
        params.clear();
        if (st) st->finish();
    }

    void Statement::cleanup() {
        if (st) {
            delete st;
            st = 0;
        }
    }

    Statement& Statement::operator,(string v) {
        bind(PARAM(v));
        return *this;
    }

    Statement& Statement::operator%(string v) {
        bind(PARAM(v));
        return *this;
    }

    Statement& Statement::operator,(long v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator%(long v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator,(double v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator%(double v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator,(dbi::null const &e) {
        bind(PARAM(e));
        return *this;
    }

    Statement& Statement::operator%(dbi::null const &e) {
        bind(PARAM(e));
        return *this;
    }

    Statement& Statement::operator,(dbi::Param v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator%(dbi::Param v) {
        bind(v);
        return *this;
    }

    Statement& Statement::operator<<(string sql) {
        params.clear();

        if (!h) throw RuntimeError("Unable to call prepare() without database handle.");
        if (st) delete st;

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

    uint32_t Statement::execute() {
        uint32_t rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    uint32_t Statement::execute(param_list_t &bind) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));
        return st->execute(bind);
    }

    uint32_t Statement::operator,(dbi::execute const &e) {
        return execute();
    }

    Result* Statement::result() {
        return new Result(st->result());
    }

    uint64_t Statement::lastInsertID() {
        return st->lastInsertID();
    }
}
