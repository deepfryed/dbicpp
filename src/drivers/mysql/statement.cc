#include "common.h"

namespace dbi {
    void MySqlStatement::init() {
        _stmt   = 0;
    }

    MySqlStatement::~MySqlStatement() {
        cleanup();
    }

    MySqlStatement::MySqlStatement(string sql, MYSQL *c) {
        init();

        conn  = c;
        _sql  = sql;
        _stmt = mysql_stmt_init(conn);

        MYSQL_PREPROCESS_QUERY(sql);
        if (mysql_stmt_prepare(_stmt, sql.c_str(), sql.length()) != 0)
            THROW_MYSQL_STMT_ERROR(_stmt);
    }

    string MySqlStatement::command() {
        return _sql;
    }

    uint32_t MySqlStatement::execute() {
        finish();
        if (mysql_stmt_execute(_stmt) != 0) THROW_MYSQL_STMT_ERROR(_stmt);
        return storeResult();
    }

    uint32_t MySqlStatement::execute(param_list_t &bind) {
        finish();

        MYSQL_BIND *params = new MYSQL_BIND[bind.size()];
        bzero(params, sizeof(MYSQL_BIND)*bind.size());

        for (int i = 0; i < bind.size(); i++) {
            params[i].buffer        = (void *)bind[i].value.data();
            params[i].buffer_length = bind[i].value.length();
            params[i].is_null       = bind[i].isnull ? &MYSQL_BOOL_TRUE : &MYSQL_BOOL_FALSE;
            params[i].buffer_type   = bind[i].isnull ? MYSQL_TYPE_NULL : MYSQL_TYPE_STRING;
        }

        if (mysql_stmt_bind_param(_stmt, params) != 0 ) {
            delete [] params;
            THROW_MYSQL_STMT_ERROR(_stmt);
        }

        if (mysql_stmt_execute(_stmt) != 0) {
            delete [] params;
            THROW_MYSQL_STMT_ERROR(_stmt);
        }

        delete [] params;
        return storeResult();
    }

    uint32_t MySqlStatement::storeResult() {
        if (mysql_stmt_store_result(_stmt) != 0 ) THROW_MYSQL_STMT_ERROR(_stmt);
        uint32_t rows = mysql_stmt_num_rows(_stmt);
        return rows ? rows : mysql_stmt_affected_rows(_stmt);
    }

    void MySqlStatement::finish() {
        if (_stmt) mysql_stmt_free_result(_stmt);
    }

    void MySqlStatement::cleanup() {
        finish();
        if (_stmt) mysql_stmt_close(_stmt);

        _stmt   = 0;
    }

    void MySqlStatement::boom(const char *m) {
        finish();
        if (MYSQL_CONNECTION_ERROR(mysql_errno(conn)))
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    MySqlBinaryResult* MySqlStatement::result() {
        return new MySqlBinaryResult(_stmt);
    }

    uint64_t MySqlStatement::lastInsertID() {
        return _stmt ? mysql_stmt_insert_id(_stmt) : 0;
    }
}
