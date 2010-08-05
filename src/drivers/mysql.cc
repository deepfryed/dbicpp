#include "dbic++.h"
#include <cstdio>
#include <mysql/mysql.h>
#include <mysql/mysql_com.h>
#include <mysql/errmsg.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define DRIVER_NAME     "mysql"
#define DRIVER_VERSION  "1.3"
#define __MYSQL_BIND_BUFFER_LEN 1024*128

#define THROW_MYSQL_STMT_ERROR(s) {\
    snprintf(errormsg, 8192, "In SQL: %s\n\n %s", _sql.c_str(), mysql_stmt_error(s));\
    boom(errormsg);\
}

namespace dbi {

    char errormsg[8192];

    char MYSQL_BOOL_TRUE  = 1;
    char MYSQL_BOOL_FALSE = 0;
    bool MYSQL_BIND_RO    = true;

    map<string, IO*> CopyInList;

    // MYSQL does not support type specific binding
    void MYSQL_PREPROCESS_QUERY(string &query) {
        int i, n = 0;
        char repl[128];
        string var;

        RE re("(?<!')(%(?:[dsfu]|l[dfu]))(?!')");
        while (re.PartialMatch(query, &var))
            re.Replace("?", &query);
    }

    void MYSQL_INTERPOLATE_BIND(MYSQL *conn, string &query, vector<Param> &bind) {
        int n = 0;
        string orig = query;
        char repl[8192];
        string var;

        RE re("(?<!')(\\?)(?!')");
        while (re.PartialMatch(query, &var)) {
            if (n >= bind.size()) {
                sprintf(errormsg, "Only %d parameters provided for query: %s", (int)bind.size(), orig.c_str());
                throw RuntimeError(errormsg);
            }
            if (bind[n].isnull) {
                strcpy(repl, "NULL");
            }
            else {
                mysql_real_escape_string(conn, repl+1, bind[n].value.c_str(), bind[n].value.length());
                repl[0] = '\'';
                repl[bind[n].value.length() + 1] = '\'';
                repl[bind[n].value.length() + 2] = 0;
                re.Replace(repl, &query);
            }
        }
    }

    void MYSQL_PROCESS_BIND(MYSQL_BIND *params, vector<Param>&bind) {
        for (int i = 0; i < bind.size(); i++) {
            params[i].buffer        = (void *)bind[i].value.data();
            params[i].buffer_length = bind[i].value.length();
            params[i].is_null       = bind[i].isnull ? &MYSQL_BOOL_TRUE : &MYSQL_BOOL_FALSE;
            params[i].buffer_type   = bind[i].isnull ? MYSQL_TYPE_NULL : MYSQL_TYPE_STRING;
        }
    }

    bool MYSQL_CONNECTION_ERROR(int error) {
        return (error == CR_SERVER_GONE_ERROR || error == CR_SERVER_LOST ||
            error == CR_SERVER_LOST_EXTENDED || error == CR_COMMANDS_OUT_OF_SYNC);
    }


    int LOCAL_INFILE_INIT(void **ptr, const char *filename, void *unused) {
        *ptr = (void*)CopyInList[filename];
        if (*ptr)
            return 0;
        else
            return -1;
    }

    int LOCAL_INFILE_READ(void *ptr, char *buffer, uint len) {
        len = ((IO*)ptr)->read(buffer, len);
        return len;
    }

    void LOCAL_INFILE_END(void *ptr) {
        for (map<string, IO*>::iterator it = CopyInList.begin(); it != CopyInList.end(); it++) {
            if (it->second == (IO*)ptr) {
                CopyInList.erase(it);
                return;
            }
        }
    }

    int LOCAL_INFILE_ERROR(void *ptr, char *error, uint len) {
        strcpy(error, ptr ? "Unknown error while bulk loading data" : "Unable to find resource to copy data.");
        return CR_UNKNOWN_ERROR;
    }


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

        void reallocateBindParam(int c, ulong length) {
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
                    params[i].length  = new ulong;
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



    class MySqlHandle;
    class MySqlStatement : public AbstractStatement {
        private:
        string _sql;
        string _uuid;
        MYSQL_STMT *_stmt;
        MySqlBind *_result;
        vector<string> _rsfields;
        vector<ulong> _buffer_lengths;
        uint _rowno, _rows, _cols;
        ResultRow _rsrow;
        ResultRowHash _rsrowhash;
        vector<int> _rstypes;
        bool _fvempty;

        protected:

        MySqlHandle *handle;
        void init();
        uint bindResultAndGetAffectedRows();
        void boom(const char*);

        public:
        ~MySqlStatement();
        MySqlStatement(string query,  MySqlHandle *h);
        void cleanup();
        string command();
        void checkReady(string m);
        uint rows();
        uint execute();
        uint execute(vector<Param> &bind);
        ulong lastInsertID();
        ResultRow& fetchRow();
        ResultRowHash& fetchRowHash();
        vector<string> fields();
        uint columns();
        bool finish();
        unsigned char* fetchValue(uint r, uint c, ulong *l = 0);
        uint currentRow();
        bool consumeResult();
        void prepareResult();
        void rewind();
        vector<int>& types();
        void seek(uint);
    };


    class MySqlResultSet : public AbstractResultSet {
        private:
        MYSQL_ROW _rowdata;
        ulong* _rowdata_lengths;
        protected:
        MySqlHandle *handle;
        MYSQL_RES *result;
        private:
        uint _rows;
        uint _cols;
        uint _rowno;
        vector<string> _rsfields;
        vector<int> _rstypes;
        ResultRow _rsrow;
        ResultRowHash _rsrowhash;
        public:
        MySqlResultSet(MySqlHandle *h);
        MySqlResultSet(MySqlHandle *h, MYSQL_RES *);
        void checkReady(string m);
        uint rows();
        uint columns();
        vector<string> fields();
        ResultRow& fetchRow();
        ResultRowHash& fetchRowHash();
        bool finish();
        unsigned char* fetchValue(uint r, uint c, ulong *l = 0);
        uint currentRow();
        void cleanup();
        ulong lastInsertID();
        bool consumeResult();
        void prepareResult();
        void rewind();
        vector<int>& types();
        void seek(uint);
        void fetchMeta(MYSQL_RES* result);
    };


    class MySqlHandle : public AbstractHandle {
        private:
        string _db;
        string _host;
        string _sql;
        MYSQL_RES *_result;
        MySqlStatement *_statement;

        protected:
        MYSQL *conn;
        int tr_nesting;
        void boom(const char*);
        void connectionError(const char *msg = 0);
        void runtimeError(const char *msg = 0);

        public:
        MySqlHandle();
        MySqlHandle(string user, string pass, string dbname, string host, string port);
        ~MySqlHandle();
        void cleanup();
        uint execute(string sql);
        uint execute(string sql, vector<Param> &bind);
        int socket();
        MySqlResultSet* aexecute(string sql, vector<Param> &bind);
        AbstractResultSet* results();
        void initAsync();
        bool isBusy();
        bool cancel();
        MySqlStatement* prepare(string sql);
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);
        void* call(string name, void* args, ulong l);
        bool close();
        void reconnect();
        ulong copyIn(string table, ResultRow &fields, IO*);
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *name);

        friend class MySqlStatement;
        friend class MySqlResultSet;
    };


    // ----------------------------------------------------------------------
    // DEFINITION
    // ----------------------------------------------------------------------

    // ----------------------------------------------------------------------
    // MySqlStatement
    // ----------------------------------------------------------------------

    void MySqlStatement::init() {
        _result = 0;
        _rowno  = 0;
        _rows   = 0;
        _cols   = 0;
        _uuid   = generateCompactUUID();
    }

    uint MySqlStatement::bindResultAndGetAffectedRows() {
        uint i;
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

    MySqlStatement::~MySqlStatement() {
        cleanup();
    }

    MySqlStatement::MySqlStatement(string query,  MySqlHandle *h) {
        init();
        handle = h;
        _sql   = query;
        _stmt  = mysql_stmt_init(handle->conn);

        MYSQL_PREPROCESS_QUERY(query);

        if (mysql_stmt_prepare(_stmt, query.c_str(), query.length()) != 0)
            THROW_MYSQL_STMT_ERROR(_stmt);

        _cols   = (uint)mysql_stmt_field_count(_stmt);

        _result = new MySqlBind(_cols);

        _buffer_lengths.reserve(_cols);
        _rsrow.reserve(_cols);
        if (_stmt->fields) {
            for (int n = 0; n < (int)_cols; n++) {
                _rsfields.push_back(_stmt->fields[n].name);
                switch(_stmt->fields[n].type) {
                    // shamelessly stolen from http://github.com/brianmario/mysql2
                    case MYSQL_TYPE_TINY:       // TINYINT field
                        _rstypes.push_back(_stmt->fields[n].length == 1 ? DBI_TYPE_BOOLEAN : DBI_TYPE_INT);
                        break;
                    case MYSQL_TYPE_SHORT:      // SMALLINT field
                    case MYSQL_TYPE_LONG:       // INTEGER field
                    case MYSQL_TYPE_INT24:      // MEDIUMINT field
                    case MYSQL_TYPE_LONGLONG:   // BIGINT field
                    case MYSQL_TYPE_YEAR:       // YEAR field
                        _rstypes.push_back(DBI_TYPE_INT);
                        break;
                    case MYSQL_TYPE_DECIMAL:    // DECIMAL or NUMERIC field
                    case MYSQL_TYPE_NEWDECIMAL: // Precision math DECIMAL or NUMERIC field (MySQL 5.0.3 and up)
                        _rstypes.push_back(DBI_TYPE_NUMERIC);
                        break;
                    case MYSQL_TYPE_FLOAT:      // FLOAT field
                    case MYSQL_TYPE_DOUBLE:     // DOUBLE or REAL field
                        _rstypes.push_back(DBI_TYPE_FLOAT);
                        break;
                    case MYSQL_TYPE_TIMESTAMP:  // TIMESTAMP field
                    case MYSQL_TYPE_DATETIME:   // DATETIME field
                        _rstypes.push_back(DBI_TYPE_TIME);
                        break;
                    case MYSQL_TYPE_TINY_BLOB:
			        case MYSQL_TYPE_MEDIUM_BLOB:
			        case MYSQL_TYPE_LONG_BLOB:
			        case MYSQL_TYPE_BLOB:
                        _rstypes.push_back(DBI_TYPE_BLOB);
                        break;
                    default:
                        _rstypes.push_back(DBI_TYPE_TEXT);
                        break;
                }
            }
        }
    }

    void MySqlStatement::cleanup() {
        finish();
        if (_stmt)
            mysql_stmt_close(_stmt);
        if(_result)
            delete _result;

        _stmt = 0;
        _result = 0;
    }

    string MySqlStatement::command() {
        return _sql;
    }

    void MySqlStatement::checkReady(string m) {
        if (!_stmt->bind_result_done)
            throw RuntimeError((m + " cannot be called yet. call execute() first").c_str());
    }

    uint MySqlStatement::rows() {
        return _rows;
    }

    uint MySqlStatement::execute() {
        int failed, tries;
        finish();

        failed = tries = 0;
        do {
            tries++;
            failed = mysql_stmt_execute(_stmt);
            if (failed && MYSQL_CONNECTION_ERROR(mysql_stmt_errno(_stmt))) {
                handle->reconnect();
                failed = mysql_stmt_execute(_stmt);
            }
        } while (failed && tries < 2);

        if (failed) THROW_MYSQL_STMT_ERROR(_stmt);

        _fvempty = true;
        _rows = bindResultAndGetAffectedRows();
        return _rows;
    }

    uint MySqlStatement::execute(vector<Param> &bind) {
        int failed, tries;
        finish();

        MySqlBind _bind(bind.size(), MYSQL_BIND_RO);
        MYSQL_PROCESS_BIND(_bind.params, bind);

        if (mysql_stmt_bind_param(_stmt, _bind.params) != 0 )
            THROW_MYSQL_STMT_ERROR(_stmt);

        failed = tries = 0;
        do {
            tries++;
            failed = mysql_stmt_execute(_stmt);
            if (failed && MYSQL_CONNECTION_ERROR(mysql_stmt_errno(_stmt))) {
                //TODO: Do we need to re-prepare here ?
                handle->reconnect();
                failed = mysql_stmt_execute(_stmt);
            }
        } while (failed && tries < 2);

        if (failed) THROW_MYSQL_STMT_ERROR(_stmt);

        _fvempty = true;
        _rows = bindResultAndGetAffectedRows();
        return _rows;
    }

    ulong MySqlStatement::lastInsertID() {
        return mysql_stmt_insert_id(_stmt);
    }

    ResultRow& MySqlStatement::fetchRow() {
        int c, rc;
        ulong length;
        checkReady("fetchRow()");
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

    ResultRowHash& MySqlStatement::fetchRowHash() {
        int c, rc;
        ulong length;
        checkReady("fetchRowHash()");

        _rsrowhash.clear();

        if (_rowno < _rows) {
            _rowno++;

            rc = mysql_stmt_fetch(_stmt);

            if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                THROW_MYSQL_STMT_ERROR(_stmt);

            for (c = 0; c < _cols; c++) {
                if (*_result->params[c].is_null) {
                    _rsrowhash[_rsfields[c]] = PARAM(null());
                }
                else {
                    length = *(_result->params[c].length);
                    if (length > _buffer_lengths[c]) {
                        if (length > _result->params[c].buffer_length)
                            _result->reallocateBindParam(c, length);
                        mysql_stmt_fetch_column(_stmt, &_result->params[c], c, 0);
                    }
                    _rsrowhash[_rsfields[c]] =
                          PARAM_BINARY((unsigned char*)_result->params[c].buffer, length);
                }
            }
        }

        return _rsrowhash;
    }

    vector<string> MySqlStatement::fields() {
        return _rsfields;
    }

    uint MySqlStatement::columns() {
        return _cols;
    }

    bool MySqlStatement::finish() {
        // free server side resultset, cursor etc.
        if (_stmt)
            mysql_stmt_free_result(_stmt);

        _rowno = _rows = 0;
        return true;
    }

    void MySqlStatement::seek(uint r) {
        checkReady("seek()");
        mysql_stmt_data_seek(_stmt, r);
        _rowno = r;
    }

    unsigned char* MySqlStatement::fetchValue(uint r, uint c, ulong *l) {
        int rc;
        ulong length;
        checkReady("fetchValue()");

        if (r >= _rows) return 0;
        if (_rowno != r || _fvempty) {
            _rowno = r;
            _fvempty = false;
            rc = mysql_stmt_fetch(_stmt);
            if (rc != 0 && rc != MYSQL_DATA_TRUNCATED)
                THROW_MYSQL_STMT_ERROR(_stmt);
        }

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

    uint MySqlStatement::currentRow() {
        return _rowno;
    }

    bool MySqlStatement::consumeResult() {
        finish();
        throw RuntimeError("consumeResult(): Incorrect API call. Use the Async API");
        return false;
    }

    void MySqlStatement::prepareResult() {
        finish();
        throw RuntimeError("prepareResult(): Incorrect API call. Use the Async API");
    }

    void MySqlStatement::boom(const char *m) {
        finish();
        if (MYSQL_CONNECTION_ERROR(mysql_errno(handle->conn)))
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    void MySqlStatement::rewind() {
        _rowno = 0;
        mysql_stmt_data_seek(_stmt, 0);
    }

    vector<int>& MySqlStatement::types() {
        return _rstypes;
    }

    // ----------------------------------------------------------------------
    // MySqlResultSet
    // ----------------------------------------------------------------------

    MySqlResultSet::MySqlResultSet(MySqlHandle *h) {
        _rows  = 0;
        _cols  = 0;
        _rowno = 0;
        handle = h;
        result = 0;
    }

    MySqlResultSet::MySqlResultSet(MySqlHandle *h, MYSQL_RES *r) {
        _rows  = 0;
        _cols  = 0;
        _rowno = 0;
        handle = h;
        result = r;
        fetchMeta(result);
    }

    void MySqlResultSet::checkReady(string m) {
        if (!result)
            throw RuntimeError((m + " cannot be called yet. ").c_str());
    }

    uint MySqlResultSet::rows() {
        return _rows;
    }

    uint MySqlResultSet::columns() {
        checkReady("columns()");
        return _cols;
    }

    vector<string> MySqlResultSet::fields() {
        checkReady("fields()");
        return _rsfields;
    }

    ResultRow& MySqlResultSet::fetchRow() {
        int n;
        checkReady("fetchRow()");
        _rsrow.clear();

        if (_rowno < _rows) {
            _rowno++;
            MYSQL_ROW row = mysql_fetch_row(result);
            ulong* lengths = mysql_fetch_lengths(result);
            for (n = 0; n < _cols; n++)
                _rsrow.push_back(row[n] == 0 ? PARAM(null()) : PARAM_BINARY((unsigned char*)row[n], lengths[n]));
        }

        return _rsrow;
    }

    ResultRowHash& MySqlResultSet::fetchRowHash() {
        int n;
        checkReady("fetchRowHash()");
        _rsrowhash.clear();
        if (_rowno < _rows) {
            _rowno++;
            MYSQL_ROW row = mysql_fetch_row(result);
            ulong* lengths = mysql_fetch_lengths(result);
            for (n = 0; n < _cols; n++)
                _rsrowhash[_rsfields[n]] = (
                    row[n] == 0 ? PARAM(null()) : PARAM_BINARY((unsigned char*)row[n], lengths[n])
                );
        }

        return _rsrowhash;
    }

    bool MySqlResultSet::finish() {
        if (result)
            mysql_free_result(result);
        result = 0;
        return true;
    }

    void MySqlResultSet::seek(uint r) {
        checkReady("seek()");
        mysql_data_seek(result, r);
        _rowno = r;
    }

    unsigned char* MySqlResultSet::fetchValue(uint r, uint c, ulong *l) {
        checkReady("fetchValue()");
        if (r >= _rows) return 0;
        if (_rowno != r || (r == 0 && c == 0)) {
            _rowno = r;
            mysql_data_seek(result, r);
            _rowdata         = mysql_fetch_row(result);
            _rowdata_lengths = mysql_fetch_lengths(result);
        }

        if (l) *l = _rowdata_lengths[c];
        return (unsigned char*)_rowdata[c];
    }

    uint MySqlResultSet::currentRow() {
        return _rowno;
    }

    void MySqlResultSet::cleanup() {
        finish();
    }

    ulong MySqlResultSet::lastInsertID() {
        checkReady("lastInsertID()");
        return mysql_insert_id(handle->conn);
    }

    // TODO Handle reconnects in consumeResult() and prepareResult()
    bool MySqlResultSet::consumeResult() {
        if (mysql_read_query_result(handle->conn) != 0) {
            finish();
            throw RuntimeError(mysql_error(handle->conn));
        }
        return false;
    }

    void MySqlResultSet::prepareResult() {
        result = mysql_store_result(handle->conn);
        fetchMeta(result);
    }

    void MySqlResultSet::fetchMeta(MYSQL_RES* result) {
        int n;
        MYSQL_FIELD *fields;

        _rows  = mysql_num_rows(result);
        _rows  = _rows > 0 ? _rows : mysql_affected_rows(handle->conn);
        _cols  = mysql_num_fields(result);

        fields = mysql_fetch_fields(result);
        for (n = 0; n < _cols; n++) {
            _rsfields.push_back(fields[n].name);
            switch(fields[n].type) {
                // shamelessly stolen from http://github.com/brianmario/mysql2
                case MYSQL_TYPE_TINY:       // TINYINT field
                    _rstypes.push_back(fields[n].length == 1 ? DBI_TYPE_BOOLEAN : DBI_TYPE_INT);
                    break;
                case MYSQL_TYPE_SHORT:      // SMALLINT field
                case MYSQL_TYPE_LONG:       // INTEGER field
                case MYSQL_TYPE_INT24:      // MEDIUMINT field
                case MYSQL_TYPE_LONGLONG:   // BIGINT field
                case MYSQL_TYPE_YEAR:       // YEAR field
                    _rstypes.push_back(DBI_TYPE_INT);
                    break;
                case MYSQL_TYPE_DECIMAL:    // DECIMAL or NUMERIC field
                case MYSQL_TYPE_NEWDECIMAL: // Precision math DECIMAL or NUMERIC field (MySQL 5.0.3 and up)
                    _rstypes.push_back(DBI_TYPE_NUMERIC);
                    break;
                case MYSQL_TYPE_FLOAT:      // FLOAT field
                case MYSQL_TYPE_DOUBLE:     // DOUBLE or REAL field
                    _rstypes.push_back(DBI_TYPE_FLOAT);
                    break;
                case MYSQL_TYPE_TIMESTAMP:  // TIMESTAMP field
                case MYSQL_TYPE_DATETIME:   // DATETIME field
                    _rstypes.push_back(DBI_TYPE_TIME);
                    break;
                default:
                    _rstypes.push_back(DBI_TYPE_TEXT);
                    break;
            }
        }
    }

    void MySqlResultSet::rewind() {
        _rowno = 0;
        mysql_data_seek(result, 0);
    }

    vector<int>& MySqlResultSet::types() {
        return _rstypes;
    }

    // ----------------------------------------------------------------------
    // MySqlHandle
    // ----------------------------------------------------------------------

    MySqlHandle::MySqlHandle() {
        tr_nesting = 0;
    }

    MySqlHandle::MySqlHandle(string user, string pass, string dbname, string host, string port) {
        uint timeout = 120;
        uint _port   = atoi(port.c_str());
        tr_nesting   = 0;
        _result      = 0;
        _statement   = 0;

        conn  = mysql_init(0);
        _db   = dbname;
        _host = host;

        if(!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(), _port, 0, 0))
            connectionError();

        mysql_set_character_set(conn, "utf8");
        mysql_options(conn, MYSQL_OPT_RECONNECT,    &MYSQL_BOOL_TRUE);
        mysql_options(conn, MYSQL_OPT_LOCAL_INFILE, 0);
        mysql_set_local_infile_handler(conn,
            LOCAL_INFILE_INIT, LOCAL_INFILE_READ, LOCAL_INFILE_END, LOCAL_INFILE_ERROR, 0);
    }

   void MySqlHandle::setTimeZoneOffset(int tzhour, int tzmin) {
        char sql[128];
        // server parses TZ style format. man timzone for more info.
        snprintf(sql, 127, "set time_zone = '%+02d:%02d'", tzhour, abs(tzmin));
        execute(sql);
    }

    void MySqlHandle::setTimeZone(char *name) {
        char sql[128];
        snprintf(sql, 127, "set time_zone = '%s'", name);
        execute(sql);
    }

    MySqlHandle::~MySqlHandle() {
        cleanup();
    }

    void MySqlHandle::cleanup() {
        // This gets called only on dlclose, so the wrapper dbi::Handle
        // closes connections and frees memory.
        if (_statement)
            delete _statement;
        if (_result)
            mysql_free_result(_result);
        if (conn)
            mysql_close(conn);
        _statement = 0;
        _result    = 0;
        conn       = 0;
    }

    uint MySqlHandle::execute(string sql) {
        int rows;
        int failed, tries;

        if (_statement) delete _statement;
        if (_result) mysql_free_result(_result);
        _sql       = sql;
        _result    = 0;
        failed     = 0;
        tries      = 0;
        _statement = 0;
        MYSQL_PREPROCESS_QUERY(sql);
        do {
            tries++;
            failed = mysql_real_query(conn, sql.c_str(), sql.length());
            if (failed && MYSQL_CONNECTION_ERROR(mysql_errno(conn))) {
                reconnect();
                failed = mysql_real_query(conn, sql.c_str(), sql.length());
            }
        } while (failed && tries < 2);

        if (failed) boom(mysql_error(conn));
        rows = mysql_affected_rows(conn);
        if (rows < 0) {
            _result = mysql_store_result(conn);
            rows = mysql_num_rows(_result);
        }
        return rows;
    }

    uint MySqlHandle::execute(string sql, vector<Param> &bind) {
        if (_statement) delete _statement;
        if (_result) mysql_free_result(_result);
        _result    = 0;
        _statement = new MySqlStatement(sql, this);
        return _statement->execute(bind);
    }

    AbstractResultSet* MySqlHandle::results() {
        AbstractResultSet *st = 0;

        if (_statement)
            st = _statement;
        else if (_result)
            st = new MySqlResultSet(this, _result);

        _statement = 0;
        _result    = 0;
        return st;
    }

    int MySqlHandle::socket() {
        return conn->net.fd;
    }

    MySqlResultSet* MySqlHandle::aexecute(string sql, vector<Param> &bind) {
        MYSQL_PREPROCESS_QUERY(sql);
        MYSQL_INTERPOLATE_BIND(conn, sql, bind);
        mysql_send_query(conn, sql.c_str(), sql.length());
        return new MySqlResultSet(this);
    }

    void MySqlHandle::initAsync() {
        // NOP
    }

    bool MySqlHandle::isBusy() {
        return false;
    }

    bool MySqlHandle::cancel() {
        return true;
    }

    MySqlStatement* MySqlHandle::prepare(string sql) {
        return new MySqlStatement(sql, this);
    }

    bool MySqlHandle::begin() {
        execute("BEGIN WORK");
        tr_nesting++;
        return true;
    };

    bool MySqlHandle::commit() {
        execute("COMMIT");
        tr_nesting--;
        return true;
    };

    bool MySqlHandle::rollback() {
        execute("ROLLBACK");
        tr_nesting--;
        return true;
    };

    bool MySqlHandle::begin(string name) {
        if (tr_nesting == 0)
            begin();
        execute("SAVEPOINT " + name);
        tr_nesting++;
        return true;
    };

    bool MySqlHandle::commit(string name) {
        execute("RELEASE SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 1)
            commit();
        return true;
    };

    bool MySqlHandle::rollback(string name) {
        execute("ROLLBACK TO SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 1)
            rollback();
        return true;
    };

    void* MySqlHandle::call(string name, void* args, ulong l) {
        return NULL;
    }

    bool MySqlHandle::close() {
        mysql_close(conn);
        conn = 0;
        return true;
    }

    void MySqlHandle::reconnect() {
        if (conn) {
            if (tr_nesting > 0)
                connectionError("Lost connection inside a transaction, unable to reconnect");
            if(mysql_ping(conn) != 0)
                runtimeError();
            else {
                sprintf(errormsg, "WARNING: Socket changed during auto reconnect to database %s on host %s\n",
                    _db.c_str(), _host.c_str());
                logMessage(_trace_fd, errormsg);
            }
        }
    }

    void MySqlHandle::boom(const char *m) {
        snprintf(errormsg, 1024, "%s - %s", m ? m : "", mysql_error(conn));
        if (MYSQL_CONNECTION_ERROR(mysql_errno(conn)))
            throw ConnectionError(errormsg);
        else
            throw RuntimeError(errormsg);
    }

    void MySqlHandle::connectionError(const char *msg) {
        snprintf(errormsg, 1024, "%s - %s", msg ? msg : "", mysql_error(conn));
        throw ConnectionError(errormsg);
    }

    void MySqlHandle::runtimeError(const char *msg) {
        snprintf(errormsg, 1024, "%s - %s", msg ? msg : "", mysql_error(conn));
        throw RuntimeError(errormsg);
    }

    ulong MySqlHandle::copyIn(string table, ResultRow &fields, IO* io) {
        int fd;
        char buffer[4096];
        string filename = generateCompactUUID();

        CopyInList[filename] = io;

        snprintf(buffer, 4095, "load data local infile '%s' replace into table %s (%s)",
            filename.c_str(), table.c_str(), fields.join(", ").c_str());
        if (_trace)
            logMessage(_trace_fd, buffer);

        if (mysql_real_query(conn, buffer, strlen(buffer))) {
            CopyInList.erase(filename);
            sprintf(buffer, "Error while copying data into table: %s - %s", table.c_str(), mysql_error(conn));
            boom(buffer);
        }

        return (ulong) mysql_affected_rows(conn);
    }
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
