#include "dbic++.h"

namespace dbi {
    Query::Query(Handle &handle, string sql) {
        h  = &handle;
        st = new Statement(handle, sql);
    }

    Query::~Query() {
        cleanup();
    }

    void Query::finish() {
        st->finish();
    }

    void Query::cleanup() {
        if (st) {
            st->cleanup();
            delete st;
            st = 0;
        }
        if (rs) {
            rs->cleanup();
            delete rs;
            rs = 0;
        }
    }

    Query& Query::operator,(string v) {
        st->bind(PARAM(v));
        return *this;
    }

    Query& Query::operator%(string v) {
        st->bind(PARAM(v));
        return *this;
    }

    Query& Query::operator,(long v) {
        st->bind(v);
        return *this;
    }

    Query& Query::operator%(long v) {
        st->bind(v);
        return *this;
    }

    Query& Query::operator,(double v) {
        st->bind(v);
        return *this;
    }

    Query& Query::operator%(double v) {
        st->bind(v);
        return *this;
    }

    Query& Query::operator,(dbi::null const &e) {
        st->bind(PARAM(e));
        return *this;
    }

    Query& Query::operator%(dbi::null const &e) {
        st->bind(PARAM(e));
        return *this;
    }

    uint32_t Query::execute() {
        uint32_t rows = st->execute();
        if (rs) { rs->cleanup(); delete rs; }
        rs = st->st->result();
        return rows;
    }

    uint32_t Query::execute(vector<Param> &bind) {
        uint32_t rows = st->execute(bind);
        if (rs) { rs->cleanup(); delete rs; }
        rs = st->st->result();
        return rows;
    }

    uint32_t Query::operator,(dbi::execute const &e) {
        uint32_t rows = st->execute();
        if (rs) { rs->cleanup(); delete rs; }
        rs = st->st->result();
        return rows;
    }

    Query& Query::operator<<(string sql) {
        if (st) {
            st->cleanup();
            delete st;
        }

        st = new Statement(h, sql);
        return *this;
    }
}
