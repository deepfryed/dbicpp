#include "common.h"

namespace dbi {
    void Sqlite3Statement::prepare() {
    }

    Sqlite3Statement::~Sqlite3Statement() {
        cleanup();
    }

    Sqlite3Statement::Sqlite3Statement(string sql,  sqlite3 *conn) {
        _sql  = sql;
        _conn = conn;

        prepare();
    }

    void Sqlite3Statement::cleanup() {
        if (_stmt)   sqlite3_finish(_stmt);
        if (_result) delete _result;

        _result = 0;
        _stmt   = 0;
    }

    void Sqlite3Statement::finish() {
        cleanup();
    }

    string Sqlite3Statement::command() {
        return _sql;
    }

    uint32_t Sqlite3Statement::execute() {
    }

    uint32_t Sqlite3Statement::execute(vector<Param> &bind) {
    }

    Sqlite3Result* Sqlite3Statement::result() {
        Sqlite3Result *instance = _result;
        _result = 0;
        return instance;
    }

    void Sqlite3Statement::boom(const char* m) {
    }

    uint64_t Sqlite3Statement::lastInsertID() {
    }
}
