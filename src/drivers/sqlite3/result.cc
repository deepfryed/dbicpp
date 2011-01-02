#include "common.h"

namespace dbi {
    void Sqlite3Result::init() {
        _result        = 0;
        _rowno         = 0;
        _rows          = 0;
        _cols          = 0;
        _bytea         = 0;
        _affected_rows = 0;
    }

    Sqlite3Result::~Sqlite3Result() {
        cleanup();
    }

    Sqlite3Result::Sqlite3Result(sqlite3_stmt *stmt, string sql) {
        init();
    }

    void Sqlite3Result::fetchMeta() {
    }

    uint32_t Sqlite3Result::rows() {
        return _affected_rows > 0 ? _affected_rows : _rows;
    }

    bool Sqlite3Result::consumeResult() {
        return false;
    }

    void Sqlite3Result::prepareResult() {
    }

    uint64_t Sqlite3Result::lastInsertID() {
        return 0;
    }

    bool Sqlite3Result::read(ResultRow &row) {
        return false;
    }

    bool Sqlite3Result::read(ResultRowHash &rowhash) {
        return false;
    }

    vector<string>& Sqlite3Result::fields() {
        return _rsfields;
    }

    uint32_t Sqlite3Result::columns() {
        return _cols;
    }

    void Sqlite3Result::cleanup() {
        _rsfields.clear();
        _rstypes.clear();

        _rowno  = 0;
        _rows   = 0;
        _result = 0;
        _bytea  = 0;
    }

    unsigned char* Sqlite3Result::read(uint32_t r, uint32_t c, uint64_t *l) {
        return 0;
    }

    uint32_t Sqlite3Result::tell() {
        return _rowno;
    }

    void Sqlite3Result::boom(const char* m) {
    }

    void Sqlite3Result::rewind() {
        _rowno = 0;
    }

    void Sqlite3Result::seek(uint32_t r) {
        _rowno = r;
    }

    vector<int>& Sqlite3Result::types() {
        return _rstypes;
    }
}
