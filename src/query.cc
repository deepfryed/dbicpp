#include "dbic++.h"

namespace dbi {
    Query::Query(Handle &handle, string sql) {
        rs = 0;
        h  = handle.h;
        st = h->prepare(sql);
    }

    Query::~Query() {
       cleanup();
    }

    void Query::cleanup() {
        finish();
        if (st) {
            delete st;
            st = 0;
        }
    }

    void Query::finish() {
        if (rs) {
            delete rs;
            rs = 0;
        }
    }

    Query& Query::operator,(string v) {
        bind(PARAM(v));
        return *this;
    }

    Query& Query::operator%(string v) {
        bind(PARAM(v));
        return *this;
    }

    Query& Query::operator,(long v) {
        bind(v);
        return *this;
    }

    Query& Query::operator%(long v) {
        bind(v);
        return *this;
    }

    Query& Query::operator,(double v) {
        bind(v);
        return *this;
    }

    Query& Query::operator%(double v) {
        bind(v);
        return *this;
    }

    Query& Query::operator,(dbi::null const &e) {
        bind(PARAM(e));
        return *this;
    }

    Query& Query::operator%(dbi::null const &e) {
        bind(PARAM(e));
        return *this;
    }

    Query& Query::operator,(dbi::Param v) {
        bind(v);
        return *this;
    }

    Query& Query::operator%(dbi::Param v) {
        bind(v);
        return *this;
    }

    uint32_t Query::execute() {
        finish();
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), params));
        uint32_t rows = st->execute(params);
        params.clear();
        rs = st->result();
        return rows;
    }

    uint32_t Query::execute(param_list_t &bind) {
        finish();
        if (_trace)
            logMessage(_trace_fd, formatParams(st->command(), bind));
        uint32_t rows = st->execute(bind);
        rs = st->result();
        return rows;
    }

    uint32_t Query::operator,(dbi::execute const &e) {
        return execute();
    }

    Query& Query::operator<<(string sql) {
        if (st) {
            st->cleanup();
            delete st;
        }

        st = h->prepare(sql);
        return *this;
    }
}
