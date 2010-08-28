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
        finish();
        if (st) {
            st->cleanup();
            delete st;
            st = 0;
        }
    }

    uint32_t Statement::rows() {
        return st->rows();
    }

    bool Statement::read(ResultRow& r) {
        return st->read(r);
    }

    bool Statement::read(ResultRowHash &r) {
        return st->read(r);
    }

    bool Statement::finish() {
        params.clear();
        return st->finish();
    }

    void Statement::cleanup() {
        if (st) st->cleanup();
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

    uint32_t Statement::columns() {
        return st->columns();
    }

    unsigned char* Statement::read(uint32_t r, uint32_t c, uint64_t *l = 0) {
        return st->read(r, c, l);
    }

    unsigned char* Statement::operator()(uint32_t r, uint32_t c) {
        return st->read(r, c, 0);
    }

    uint32_t Statement::tell() {
        return st->tell();
    }

    vector<int>& Statement::types() {
        return st->types();
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

    uint32_t Statement::operator,(dbi::execute const &e) {
        uint32_t rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    uint64_t Statement::lastInsertID() {
        return st->lastInsertID();
    }

    void Statement::rewind() {
        st->rewind();
    }

    void Statement::seek(uint32_t r) {
        st->seek(r);
    }
}
