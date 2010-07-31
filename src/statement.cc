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

    uint Statement::rows() {
        return st->rows();
    }

    ResultRow& Statement::fetchRow() {
        return st->fetchRow();
    }

    ResultRowHash& Statement::fetchRowHash() {
        return st->fetchRowHash();
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

    uint Statement::columns() {
        return st->columns();
    }

    unsigned char* Statement::fetchValue(uint r, uint c, ulong *l = 0) {
        return st->fetchValue(r, c, l);
    }

    unsigned char* Statement::operator()(uint r, uint c) {
        return st->fetchValue(r, c, 0);
    }

    uint Statement::currentRow() {
        return st->currentRow();
    }

    vector<int>& Statement::types() {
        return st->types();
    }

    uint Statement::execute() {
        uint rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    uint Statement::execute(ResultRow &bind) {
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));
        return st->execute(bind);
    }

    uint Statement::operator,(dbi::execute const &e) {
        uint rc;
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        rc = st->execute(params);
        params.clear();
        return rc;
    }

    ulong Statement::lastInsertID() {
        return st->lastInsertID();
    }

    void Statement::rewind() {
        st->rewind();
    }

    void Statement::seek(uint r) {
        st->seek(r);
    }
}
