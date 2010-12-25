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
        return st->finish();
    }

    void Statement::cleanup() {
        if (st) {
            st->cleanup();
            delete st;
            st = 0;
        }
    }

    // syntactic sugar.
    Statement Handle::operator<<(string sql) {
        return Statement(h->prepare(sql));
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

    Statement& Statement::operator<<(string sql) {
        params.clear();

        if (st) { st->cleanup(); delete st; }
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

    uint32_t Statement::execute() {
        uint32_t rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    uint32_t Statement::execute(vector<Param> &bind) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));
        return st->execute(bind);
    }

    Result* Statement::query() {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));

        AbstractResult *rs = st->query(params);
        params.clear();
        return new Result(rs);
    }

    Result* Statement::query(vector<Param> &bind) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));

        AbstractResult *rs = st->query(bind);
        return new Result(rs);
    }

    uint32_t Statement::operator,(dbi::execute const &e) {
        uint32_t rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    Result* Statement::operator,(dbi::query const &e) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        AbstractResult *rs = st->query(params);
        params.clear();
        return new Result(rs);
    }
}
