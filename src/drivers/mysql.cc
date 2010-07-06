#include "dbic++.h"
#include <cstdio>
#include <mysql/mysql.h>
#include <mysql/mysql_com.h>

#define DRIVER_NAME     "mysql"
#define DRIVER_VERSION  "1.0.1"

#define THROW_MYSQL_STMT_ERROR(s) {\
    fprintf(stderr, "In SQL: %s\n\n", _sql.c_str());\
    throw RuntimeError(mysql_stmt_error(s));\
}

namespace dbi {

    char MYSQL_BOOL_TRUE  = 1;
    char MYSQL_BOOL_FALSE = 0;
    bool MYSQL_BIND_RO    = true;

    // MYSQL does not support type specific binding
    void mysqlPreProcessQuery(string &query) {
        int i, n = 0;
        char repl[128];
        string var;

        RE re("(?<!')(%(?:[dsfu]|l[dfu]))(?!')");
        while (re.PartialMatch(query, &var))
            re.Replace("?", &query);
    }

    void mysqlProcessBindParams(MYSQL_BIND *params, vector<Param>&bind) {
        for (int i = 0; i < bind.size(); i++) {
            params[i].buffer        = (void *)bind[i].value.data();
            params[i].buffer_length = bind[i].value.length();
            params[i].is_null       = bind[i].isnull ? &MYSQL_BOOL_TRUE : &MYSQL_BOOL_FALSE;
            params[i].buffer_type   = bind[i].isnull ? MYSQL_TYPE_NULL : MYSQL_TYPE_STRING;
        }
    }


    #define __MYSQL_BIND_BUFFER_LEN 1024*128

    class MySqlBind {
        protected:
        bool ro_buffer;
        public:
        int count;
        MYSQL_BIND *params;

        MySqlBind(int n) {
            count = n;
            ro_buffer = false;
            allocateBindParams();
        }

        MySqlBind(int n, bool ro_flag) {
            count = n;
            ro_buffer = ro_flag;
            allocateBindParams();
        }

        MySqlBind()  {
            ro_buffer = false;
            count = 0;
            params = 0;
        }

        ~MySqlBind() {
            deallocateBindParams();
        }

        void reallocateBindParam(int c, unsigned long length) {
            if (!ro_buffer) {
                delete [] (unsigned char*)params[c].buffer;
                params[c].buffer  = (void*)new unsigned char[length];
                params[c].buffer_length = length;
            }
        }

        private:
        void allocateBindParams() {
            params = new MYSQL_BIND[count];
            memset(params, 0, sizeof(MYSQL_BIND)*count);

            if (!ro_buffer) {
                for (int i = 0; i < count; i++) {
                    params[i].length  = new unsigned long;
                    params[i].is_null = new char;
                    params[i].buffer  = (void*)new unsigned char[__MYSQL_BIND_BUFFER_LEN];
                    params[i].buffer_length = __MYSQL_BIND_BUFFER_LEN;
                }
            }
        }

        void deallocateBindParams() {
            if (!ro_buffer) {
                for (int i = 0; i < count; i++) {
                    delete params[i].length;
                    delete params[i].is_null;
                    delete [] (unsigned char*)params[i].buffer;
                }
            };

            delete [] params;
        }
    };


    class MySqlStatement : public AbstractStatement {
        private:
        string _sql;
        string _uuid;
        MYSQL *_conn;
        MYSQL_STMT *_stmt;
        MySqlBind *_result;
        vector<string> _result_fields;
        vector<unsigned long> _buffer_lengths;
        unsigned int _rowno, _rows, _cols;
        ResultRow _rsrow;
        ResultRowHash _rsrowhash;

        protected:

        void init() {
            _result = 0;
            _rowno  = 0;
            _rows   = 0;
            _cols   = 0;
            _uuid   = generateCompactUUID();
        }

        unsigned int bindResultAndGetAffectedRows() {
            unsigned int i;
            MYSQL_RES *res = mysql_stmt_result_metadata(_stmt);

            if (res) {
                if (!_stmt->bind_result_done) {
                    _buffer_lengths.clear();

                    for (i = 0; i < _cols; i++)
                        _buffer_lengths.push_back(_result->params[i].buffer_length);

                    if (mysql_stmt_bind_result(_stmt, _result->params) != 0) {
                        mysql_free_result(res);
                        THROW_MYSQL_STMT_ERROR(_stmt);
                    }
                }

                if (mysql_stmt_store_result(_stmt) != 0 ) {
                    mysql_free_result(res);
                    THROW_MYSQL_STMT_ERROR(_stmt);
                }

                mysql_free_result(res);
            }

            return res ? mysql_stmt_num_rows(_stmt) : mysql_stmt_affected_rows(_stmt);
        }

        public:
        MySqlStatement() {}
        ~MySqlStatement() {
            finish();
            cleanup();
        }

        MySqlStatement(string query,  MYSQL *c) {
            init();
            _conn  = c;
            _sql   = query;
            _stmt  = mysql_stmt_init(c);

            mysqlPreProcessQuery(query);

            if (mysql_stmt_prepare(_stmt, query.c_str(), query.length()) != 0)
                THROW_MYSQL_STMT_ERROR(_stmt);

            _cols   = (unsigned int)mysql_stmt_field_count(_stmt);

            _result = new MySqlBind(_cols);

            _buffer_lengths.reserve(_cols);
            _rsrow.reserve(_cols);
        }

        void cleanup() {
            if (_stmt)
                mysql_stmt_close(_stmt);
            if(_result)
                delete _result;

            _stmt = 0;
            _result = 0;
        }

        string command() {
            return _sql;
        }

        void check_ready(string m) {
            if (!_stmt->bind_result_done)
                throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
        }

        unsigned int rows() {
            check_ready("rows()");
            return _rows;
        }

        unsigned int execute() {
            _result_fields.clear();
            finish();

            if (mysql_stmt_execute(_stmt) != 0)
                THROW_MYSQL_STMT_ERROR(_stmt);

            _rows = bindResultAndGetAffectedRows();
            return _rows;
        }

        unsigned int execute(vector<Param> &bind) {
            _result_fields.clear();
            finish();

            MySqlBind _bind(bind.size(), MYSQL_BIND_RO);
            mysqlProcessBindParams(_bind.params, bind);

            if (mysql_stmt_bind_param(_stmt, _bind.params) != 0 )
                THROW_MYSQL_STMT_ERROR(_stmt);

            if (mysql_stmt_execute(_stmt) != 0)
                THROW_MYSQL_STMT_ERROR(_stmt);
            _rows = bindResultAndGetAffectedRows();

            return _rows;
        }

        unsigned long lastInsertID() {
            return mysql_stmt_insert_id(_stmt);
        }

        ResultRow& fetchRow() {
            int c, rc;
            unsigned long length;
            check_ready("fetchRow()");
            _rsrow.clear();

            if (_rowno < _rows) {
                _rowno++;

                rc = mysql_stmt_fetch(_stmt);

                if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                    THROW_MYSQL_STMT_ERROR(_stmt);

                for (c = 0; c < _cols; c++) {
                    if (*_result->params[c].is_null) {
                        _rsrow.push_back(PARAM(null()));
                    }
                    else {
                        length = *(_result->params[c].length);
                        if (length > _buffer_lengths[c]) {
                            if (length > _result->params[c].buffer_length)
                                _result->reallocateBindParam(c, length);
                            mysql_stmt_fetch_column(_stmt, &_result->params[c], c, 0);
                        }
                        _rsrow.push_back(PARAM_BINARY((unsigned char*)_result->params[c].buffer, length));
                    }
                }
            }

            return _rsrow;
        }

        ResultRowHash& fetchRowHash() {
            int c, rc;
            unsigned long length;
            check_ready("fetchRowHash()");

            _rsrowhash.clear();

            if (_rowno < _rows) {
                _rowno++;

                if (_result_fields.size() == 0)
                    fields();

                rc = mysql_stmt_fetch(_stmt);

                if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                    THROW_MYSQL_STMT_ERROR(_stmt);

                for (c = 0; c < _cols; c++) {
                    if (*_result->params[c].is_null) {
                        _rsrowhash[_result_fields[c]] = PARAM(null());
                    }
                    else {
                        length = *(_result->params[c].length);
                        if (length > _buffer_lengths[c]) {
                            if (length > _result->params[c].buffer_length)
                                _result->reallocateBindParam(c, length);
                            mysql_stmt_fetch_column(_stmt, &_result->params[c], c, 0);
                        }
                        _rsrowhash[_result_fields[c]] =
                              PARAM_BINARY((unsigned char*)_result->params[c].buffer, length);
                    }
                }
            }

            return _rsrowhash;
        }

        vector<string> fields() {
            MYSQL_RES *res = mysql_stmt_result_metadata(_stmt);
            if (res) {
                MYSQL_FIELD *fields = mysql_fetch_fields(res);
                for (unsigned int i = 0; i < _cols; i++)
                    _result_fields.push_back(fields[i].name);
                mysql_free_result(res);
            }
            return _result_fields;
        }

        unsigned int columns() {
            return _cols;
        }

        bool finish() {
            // free server side resultset, cursor etc.
            if (_stmt)
                mysql_stmt_free_result(_stmt);

            _rowno = _rows = 0;
            return true;
        }

        unsigned char* fetchValue(int r, int c, unsigned long *l = 0) {
            int rc;
            unsigned long length;
            check_ready("fetchValue()");

            mysql_stmt_data_seek(_stmt, r);
            rc = mysql_stmt_fetch(_stmt);
            if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                THROW_MYSQL_STMT_ERROR(_stmt);

            length = *(_result->params[c].length);
            if (*_result->params[c].is_null) {
                return 0;
            }
            else {
                if (l) *l = length;
                if (length > _buffer_lengths[c]) {
                    if (length > _result->params[c].buffer_length)
                        _result->reallocateBindParam(c, length);
                    mysql_stmt_fetch_column(_stmt, &_result->params[c], c, 0);
                }
                return (unsigned char*)_result->params[c].buffer;
            }
        }

        unsigned int currentRow() {
            return _rowno;
        }

        void advanceRow() {
            _rowno = _rowno <= _rows ? _rowno + 1 : _rowno;
        }
    };

    class MySqlHandle : public AbstractHandle {
        protected:
        MYSQL *conn;
        int tr_nesting;

        public:
        MySqlHandle() {
            tr_nesting = 0;
        }

        MySqlHandle(string user, string pass, string dbname, string host, string port) {
            unsigned int _port = atoi(port.c_str());
            tr_nesting = 0;

            conn = mysql_init(0);

            if(!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(), _port, 0, 0))
                throw ConnectionError(mysql_error(conn));
        }

        ~MySqlHandle() {
            cleanup();
        }

        void cleanup() {
            // This gets called only on dlclose, so the wrapper dbi::Handle
            // closes connections and frees memory.
            if (conn)
                mysql_close(conn);
            conn = 0;
        }

        unsigned int execute(string sql) {
            if (mysql_real_query(conn, sql.c_str(), sql.length()) != 0)
                throw RuntimeError(mysql_error(conn));

            return mysql_affected_rows(conn);
        }

        unsigned int execute(string sql, vector<Param> &bind) {
            MySqlStatement st(sql, conn);
            return st.execute(bind);
        }

        MySqlStatement* prepare(string sql) {
            return new MySqlStatement(sql, conn);
        }

        bool begin() {
            execute("BEGIN WORK");
            tr_nesting++;
            return true;
        };

        bool commit() {
            execute("COMMIT");
            tr_nesting--;
            return true;
        };

        bool rollback() {
            execute("ROLLBACK");
            tr_nesting--;
            return true;
        };

        bool begin(string name) {
            if (tr_nesting == 0)
                begin();
            execute("SAVEPOINT " + name);
            tr_nesting++;
            return true;
        };

        bool commit(string name) {
            execute("RELEASE SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1)
                commit();
            return true;
        };

        bool rollback(string name) {
            execute("ROLLBACK TO SAVEPOINT " + name);
            tr_nesting--;
            if (tr_nesting == 1)
                rollback();
            return true;
        };

        void* call(string name, void* args) {
            return NULL;
        }

        bool close() {
            mysql_close(conn);
            conn = 0;
            return true;
        }
    };
}

using namespace std;
using namespace dbi;

extern "C" {
    MySqlHandle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        if (host == "")
            host = "127.0.0.1";
        if (port == "0" || port == "")
            port = "3306";

        return new MySqlHandle(user, pass, dbname, host, port);
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
