#include "common.h"

namespace dbi {
    void MySqlStatement::init() {
        _stmt   = 0;
        _result = 0;
        _uuid   = generateCompactUUID();
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

    uint32_t MySqlStatement::execute(vector<Param> &bind) {
        finish();

        MySqlBind mysqlbind(bind.size(), MYSQL_BIND_RO);
        MYSQL_PROCESS_BIND(mysqlbind.params, bind);

        if (mysql_stmt_bind_param(_stmt, mysqlbind.params) != 0 ) THROW_MYSQL_STMT_ERROR(_stmt);
        if (mysql_stmt_execute(_stmt) != 0)                       THROW_MYSQL_STMT_ERROR(_stmt);

        return storeResult();
    }

    uint32_t MySqlStatement::storeResult() {
        MYSQL_RES *res = mysql_stmt_result_metadata(_stmt);

        if (res) {
            mysql_free_result(res);
            if (!_stmt->bind_result_done) {
                _result = new MySqlBind(_stmt->field_count, MYSQL_BIND_RW);
                if (mysql_stmt_bind_result(_stmt, _result->params) != 0) THROW_MYSQL_STMT_ERROR(_stmt);
            }
            if (mysql_stmt_store_result(_stmt) != 0 ) THROW_MYSQL_STMT_ERROR(_stmt);
        }

        return res ? mysql_stmt_num_rows(_stmt) : mysql_stmt_affected_rows(_stmt);
    }

    void MySqlStatement::finish() {
        if (_stmt) mysql_stmt_free_result(_stmt);
    }

    void MySqlStatement::cleanup() {
        if (_stmt)   mysql_stmt_close(_stmt);
        if (_result) delete _result;

        _stmt   = 0;
        _result = 0;
    }

    void MySqlStatement::boom(const char *m) {
        finish();
        if (MYSQL_CONNECTION_ERROR(mysql_errno(conn)))
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    MySqlStatementResult* MySqlStatement::result() {
        return _result ?
            new MySqlStatementResult(_stmt, _result->params, _sql) :
            new MySqlStatementResult(_stmt, 0, _sql);
    }

    uint64_t MySqlStatement::lastInsertID() {
        return _stmt ? mysql_stmt_insert_id(_stmt) : 0;
    }
}
