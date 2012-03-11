#include "dbic++.h"

namespace dbi {

    Result::Result() { rs = 0; }

    Result::Result(AbstractResult *ars) {
        rs = ars; // :P
    }

    Result::~Result() {
        cleanup();
    }

    uint32_t Result::rows() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->rows();
    }

    bool Result::read(ResultRow& r) {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->read(r);
    }

    bool Result::read(ResultRowHash &r) {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->read(r);
    }

    void Result::cleanup() {
        if (rs) {
            delete rs;
            rs = 0;
        }
    }

    string_list_t& Result::fields() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->fields();
    }

    uint32_t Result::columns() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->columns();
    }

    unsigned char* Result::read(uint32_t r, uint32_t c, uint64_t *l = 0) {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->read(r, c, l);
    }

    unsigned char* Result::operator()(uint32_t r, uint32_t c) {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->read(r, c, 0);
    }

    uint32_t Result::tell() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->tell();
    }

    int_list_t& Result::types() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->types();
    }

    uint64_t Result::lastInsertID() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        return rs->lastInsertID();
    }

    void Result::rewind() {
        if (!rs) throw RuntimeError("Invalid Result instance");
        rs->rewind();
    }

    void Result::seek(uint32_t r) {
        if (!rs) throw RuntimeError("Invalid Result instance");
        rs->seek(r);
    }

    void Result::retrieve() {
        while (rs->consumeResult()) {
            printf("retr\n");
        }
        rs->prepareResult();
    }
}
