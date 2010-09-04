#include "dbic++.h"
#include "sqlcli1.h"

#define DRIVER_NAME     "db2"
#define DRIVER_VERSION  "1.3"


namespace dbi {

    using namespace std;

    void db2error(short type, SQLHANDLE handle) {
        SQLCHAR     message[SQL_MAX_MESSAGE_LENGTH+1];
        SQLCHAR     sqlstate[SQL_SQLSTATE_SIZE+1];
        SQLINTEGER  sqlcode;
        SQLSMALLINT length, i = 1;

        string messages;

        while(SQLGetDiagRec(type, handle, i++, sqlstate, &sqlcode, message, SQL_MAX_MESSAGE_LENGTH + 1, &length) == 0)
            messages += string((char*)message) + "\n";
        throw RuntimeError(messages);
    }

    void CHECK_HANDLE_RESULT(SQLHANDLE handle, int rc) {
        if (rc != SQL_SUCCESS) {
            SQLEndTran(SQL_HANDLE_DBC, handle, SQL_ROLLBACK);
            db2error(SQL_HANDLE_DBC, handle);
        }
    }

    void CHECK_STATEMENT_RESULT(SQLHANDLE handle, SQLHANDLE stmt, int rc) {
        if (rc != SQL_SUCCESS) {
            SQLEndTran(SQL_HANDLE_DBC, handle, SQL_ROLLBACK);
            db2error(SQL_HANDLE_STMT, stmt);
        }
    }

    class DB2Statement : public AbstractStatement {
        protected:
        string sql;
        uint32_t _rowno, _rows;
        vector<ResultRow> results;
        vector<string>    _rsfields;
        vector<int>       _rstypes;
        SQLHANDLE stmt;
        SQLSMALLINT _columns;
        SQLHANDLE handle;
        void checkResult(int);
        void fetchMeta();

        public:
        uint32_t columns();
        uint32_t rows();
        vector<string> fields();
        bool read(ResultRow &);
        bool read(ResultRowHash &);
        unsigned char* read(uint32_t r, uint32_t c, uint64_t *l = 0);
        bool finish();
        uint32_t tell();
        void cleanup();
        uint64_t lastInsertID();
        bool consumeResult();
        void prepareResult();
        void rewind();
        vector<int>& types();
        void seek(uint32_t);

        // Statement specific stuff
        DB2Statement(SQLHANDLE, string);
        DB2Statement(SQLHANDLE);
        ~DB2Statement();
        uint32_t execute();
        uint32_t execute(vector<Param> &bind);
        uint32_t execute(string);
        uint32_t execute(string, vector<Param> &bind);
        string command();
    };

    class DB2Handle : public AbstractHandle {
        protected:
        SQLHANDLE env, handle;
        DB2Statement *stmt;
        void setup();
        void TCPIPConnect(string, string, string, string, string);
        int tr_nesting;
        void checkResult(int);

        public:
        DB2Handle();
        DB2Handle(string, string, string);
        DB2Handle(string, string, string, string, string);
        ~DB2Handle();
        uint32_t execute(string);
        uint32_t execute(string, vector<Param> &);
        DB2Statement* prepare(string);
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string);
        bool commit(string);
        bool rollback(string);
        void* call(string, void*, uint64_t);
        bool close();
        void reconnect();
        uint64_t write(string, FieldSet&, IO*);
        void setTimeZoneOffset(int, int);
        void setTimeZone(char*);
        void cleanup();
        string escape(string);
        AbstractResult* results();

        // ASYNC
        int socket();
        AbstractResult* aexecute(string, vector<Param> &);
        void initAsync();
        bool isBusy();
        bool cancel();
    };


    // -----------------------------------------------------------------------------
    // DB2Statement
    // -----------------------------------------------------------------------------

    void DB2Statement::checkResult(int rc) {
        CHECK_STATEMENT_RESULT(handle, stmt, rc);
    }

    DB2Statement::DB2Statement(SQLHANDLE handle, string sql) {
        _rowno = _rows = 0;
        this->handle = handle;
        this->sql    = sql;
        SQLAllocHandle(SQL_HANDLE_STMT, handle, &stmt);
        checkResult(SQLPrepare(stmt, (SQLCHAR*)sql.c_str(), SQL_NTS));
        fetchMeta();
    }

    DB2Statement::DB2Statement(SQLHANDLE handle) {
        _rowno = _rows = 0;
        this->handle = handle;
        SQLAllocHandle(SQL_HANDLE_STMT, handle, &stmt);
    }

    void DB2Statement::fetchMeta() {
        int i, j;
        SQLCHAR buffer[1024];
        SQLSMALLINT type, length;
        SQLNumResultCols(stmt, &_columns);

        for (i = 0; i < _columns; i++) {
            SQLDescribeCol(stmt, i+1, buffer, 1024, &length, &type, 0, 0, 0);
            for (j = 0; j < length; j++) buffer[j] = tolower(buffer[j]);
            _rsfields.push_back(string((char*)buffer, length));
            switch(type) {
                case SQL_BIGINT:
                case SQL_INTEGER:
                case SQL_SMALLINT:
                    _rstypes.push_back(DBI_TYPE_INT); break;

                case SQL_BINARY:
                case SQL_BLOB:
                case SQL_BLOB_LOCATOR:
                case SQL_VARBINARY:
                    _rstypes.push_back(DBI_TYPE_BLOB); break;

                case SQL_DECIMAL:
                case SQL_DOUBLE:
                case SQL_NUMERIC:
                case SQL_REAL:
                    _rstypes.push_back(DBI_TYPE_NUMERIC); break;

                case SQL_DECFLOAT:
                case SQL_FLOAT:
                    _rstypes.push_back(DBI_TYPE_FLOAT); break;

                case SQL_TYPE_DATE:
                    _rstypes.push_back(DBI_TYPE_DATE); break;

                case SQL_TYPE_TIMESTAMP:
                    _rstypes.push_back(DBI_TYPE_TIMESTAMP); break;

                /*
                case SQL_GRAPHIC:
                case SQL_LONGVARBINARY:
                case SQL_LONGVARCHAR:
                case SQL_LONGVARGRAPHIC:
                case SQL_TYPE_TIME:
                case SQL_VARGRAPHIC:
                case SQL_XML:
                case SQL_CHAR:
                case SQL_CLOB:
                case SQL_CLOB_LOCATOR:
                case SQL_DBCLOB:
                case SQL_DBCLOB_LOCATOR:
                case SQL_VARCHAR:
                */
                default:
                    _rstypes.push_back(DBI_TYPE_TEXT); break;
            }
        }
    }

    uint32_t DB2Statement::rows() {
        return _rows;
    }

    uint32_t DB2Statement::columns() {
        return _columns;
    }

    vector<string> DB2Statement::fields() {
        return _rsfields;
    }

    bool DB2Statement::read(ResultRow &r) {
        if (_rowno < _rows) {
            r = results[_rowno++];
            return true;
        }
        return false;
    }

    bool DB2Statement::read(ResultRowHash &r) {
        if (_rowno < _rows) {
            r.clear();
            for (int i = 0; i < _columns; i++)
                r[_rsfields[i]] = results[_rowno][i];
            _rowno++;
            return true;
        }
        return false;
    }

    unsigned char* DB2Statement::read(uint32_t r, uint32_t c, uint64_t *l) {
        if (r < _rows && c < _columns) {
            if (l) *l = results[r][c].value.length();
            return results[r][c].isnull ? 0 : (unsigned char*)results[r][c].value.data();
        }
        return 0;
    }

    bool DB2Statement::finish() {
        SQLCloseCursor(stmt);
        return true;
    }

    uint32_t DB2Statement::tell() {
        return _rowno;
    }

    void DB2Statement::seek(uint32_t r) {
        _rowno = r >= 0 && r < _rows ? r : _rowno;
    }

    void DB2Statement::rewind() {
        _rowno = 0;
    }

    vector<int>& DB2Statement::types() {
        return _rstypes;
    }

    // TODO NULL
    bool DB2Statement::consumeResult() {
        ResultRow r;
        SQLINTEGER length;

        int rc = SQLFetch(stmt);
        if (rc == SQL_ERROR || rc == SQL_NO_DATA_FOUND)
            return true;

        unsigned char *buffer = new unsigned char[8192];
        uint64_t buffer_length = 8192;

        while (rc != SQL_ERROR && rc != SQL_NO_DATA_FOUND) {
            r.clear();
            for (int i = 0; i < _columns; i++ ) {
                SQLGetData(stmt, i+1, SQL_C_CHAR, buffer, buffer_length, &length);
                if (length > buffer_length) {
                    delete [] buffer;
                    buffer = new unsigned char[length + 1];
                    buffer_length = length + 1;
                    SQLGetData(stmt, i+1, SQL_C_CHAR, buffer, buffer_length, &length);
                }
                r.push_back(_rstypes[i] == DBI_TYPE_BLOB ? PARAM_BINARY(buffer, length) : PARAM(buffer, length));
            }
            results.push_back(r);
            _rows++;
            rc = SQLFetch(stmt);
        }

        delete [] buffer;
        return true;
    }

    void DB2Statement::prepareResult() {
        _rows = _rowno = 0;
        results.clear();
    }

    // NOTE
    // You can only call this after doing an insert with a select as below,
    // select id from final table (insert into ...)
    uint64_t DB2Statement::lastInsertID() {
        if (_rows > 0)
            return atol(results[0][0].value.c_str());
        return 0;
    }

    uint32_t DB2Statement::execute() {
        SQLINTEGER cmdrows = 0;
        finish();
        prepareResult();
        checkResult(SQLExecute(stmt));
        SQLRowCount(stmt, &cmdrows);
        consumeResult();
        return cmdrows > 0 ? cmdrows : _rows;
    }

    uint32_t DB2Statement::execute(string sql) {
        SQLINTEGER cmdrows = 0;
        finish();
        this->sql = sql;
        prepareResult();
        checkResult(SQLExecDirect(stmt, (SQLCHAR*)sql.c_str(), sql.length()));
        fetchMeta();
        SQLRowCount(stmt, &cmdrows);
        consumeResult();
        return cmdrows > 0 ? cmdrows : _rows;
    }

    // TODO BLOB
    uint32_t DB2Statement::execute(vector<Param> &bind) {
        int i, rc;
        for (i = 0; i < bind.size(); i++) {
            rc = SQLBindParameter(
                stmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 3, (void*)bind[i].value.data(), 8192, 0);
            checkResult(rc);
        }
        return execute();
    }

    // TODO BLOB
    uint32_t DB2Statement::execute(string sql, vector<Param> &bind) {
        int i, rc;
        finish();
        for (i = 0; i < bind.size(); i++) {
            rc = SQLBindParameter(
                stmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 3, (void*)bind[i].value.data(), 8192, 0);
            checkResult(rc);
        }
        return execute(sql);
    }

    void DB2Statement::cleanup() {
        if (stmt)
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        stmt = 0;
    }

    DB2Statement::~DB2Statement() {
        cleanup();
    }

    string DB2Statement::command() {
        return sql;
    }


    // -----------------------------------------------------------------------------
    // DB2Handle
    // -----------------------------------------------------------------------------

    DB2Handle::DB2Handle() { }

    void DB2Handle::checkResult(int rc) {
        CHECK_HANDLE_RESULT(handle, rc);
    }

    // TODO error checks.
    void DB2Handle::setup() {
        stmt = 0;
        tr_nesting = 0;
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &handle);
    }

    void DB2Handle::TCPIPConnect(string user, string pass, string dbname, string host, string port) {
        string dsn = "DRIVER={IBM DB2 ODBC DRIVER};DATABASE=" + dbname +
                     ";HOSTNAME=127.0.0.1;PORT=50000;PROTOCOL=TCPIP;";

        if (user != "")
            dsn += "UID=" + user + ";PWD=" + pass;

        int rc = SQLDriverConnect(
            handle, 0,
            (SQLCHAR*)dsn.c_str(),    SQL_NTS,
            0, 0, 0, SQL_DRIVER_NOPROMPT
        );
        checkResult(rc);
    }

    DB2Handle::DB2Handle(string user, string pass, string dbname) {
        setup();

        if (user == string(getlogin()) && pass.length() == 0)
            user = "";

        int rc = SQLConnect(
            handle,
            (SQLCHAR*)dbname.c_str(), SQL_NTS,
            (SQLCHAR*)user.c_str(),   SQL_NTS,
            (SQLCHAR*)pass.c_str(),   SQL_NTS
        );
        checkResult(rc);
        SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (void*)"on", 2);
    }

    DB2Handle::DB2Handle(string user, string pass, string dbname, string host, string port) {
        setup();
        TCPIPConnect(user, pass, dbname, host, port);
        SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (void*)"on", 2);
    }

    DB2Statement* DB2Handle::prepare(string sql) {
        return new DB2Statement(handle, sql);
    }

    uint32_t DB2Handle::execute(string sql) {
        if (stmt) { stmt->cleanup(); delete stmt; }
        stmt = new DB2Statement(handle);
        return stmt->execute(sql);
    }

    uint32_t DB2Handle::execute(string sql, vector<Param> &bind) {
        if (stmt) { stmt->cleanup(); delete stmt; }
        stmt = new DB2Statement(handle);
        return stmt->execute(sql, bind);
    }

    void DB2Handle::cleanup() {
        if (stmt) { stmt->cleanup(); delete stmt; }
        stmt = 0;
        SQLDisconnect(handle);
        SQLFreeHandle(SQL_HANDLE_DBC, handle);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
    }

    DB2Handle::~DB2Handle() {
        cleanup();
    }

    string DB2Handle::escape(string value) {
        SQLINTEGER length;
        SQLCHAR *buffer = (SQLCHAR*)malloc(value.length()*2+1);
        SQLNativeSql(handle, (SQLCHAR*)value.data(), value.length(), buffer, value.length()*2, &length);
        value = string((char*)buffer, length);
        free(buffer);
        return value;
    }

    // TODO
    uint64_t DB2Handle::write(string table, FieldSet &fields, IO* io) {
        return 0;
    }

    bool DB2Handle::begin() {
        SQLEndTran(SQL_HANDLE_DBC, handle, SQL_ROLLBACK);
        SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (void*)"off", 3);
        tr_nesting = 1;
        return true;
    }

    bool DB2Handle::rollback() {
        SQLEndTran(SQL_HANDLE_DBC, handle, SQL_ROLLBACK);
        SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (void*)"on", 2);
        tr_nesting = 0;
        return true;
    }

    bool DB2Handle::commit() {
        SQLEndTran(SQL_HANDLE_DBC, handle, SQL_COMMIT);
        SQLSetConnectAttr(handle, SQL_ATTR_AUTOCOMMIT, (void*)"on", 2);
        tr_nesting = 0;
        return true;
    }

    bool DB2Handle::begin(string name) {
        if (tr_nesting == 0) {
            begin();
            tr_nesting = 0;
        }
        string sql = ("SAVEPOINT " + name);
        checkResult(SQLExecDirect(handle, (SQLCHAR*)sql.data(), sql.length()));
        tr_nesting++;
        return true;
    }

    bool DB2Handle::rollback(string name) {
        string sql = ("ROLLBACK TO SAVEPOINT " + name);
        checkResult(SQLExecDirect(handle, (SQLCHAR*)sql.data(), sql.length()));
        tr_nesting--;
        if (tr_nesting == 0) rollback();
        return true;
    }

    bool DB2Handle::commit(string name) {
        string sql = ("RELEASE SAVEPOINT " + name);
        checkResult(SQLExecDirect(handle, (SQLCHAR*)sql.data(), sql.length()));
        tr_nesting--;
        if (tr_nesting == 0) commit();
        return true;
    }

    void* DB2Handle::call(string name, void* args, uint64_t len) {
        return 0;
    }

    // TODO
    void DB2Handle::reconnect() {
    }

    // NOT SUPPORTED
    void DB2Handle::setTimeZoneOffset(int hour, int min) {
        throw RuntimeError("DB2 unsupported API setTimeZoneOffset");
    }

    // NOT SUPPORTED
    void DB2Handle::setTimeZone(char *name) {
        throw RuntimeError("DB2 unsupported API setTimeZone");
    }


    AbstractResult* DB2Handle::results() {
        AbstractResult *res = stmt;
        stmt = 0;
        return res;
    }

    // TODO
    int DB2Handle::socket() {
        return 0;
    }

    // TODO
    AbstractResult* DB2Handle::aexecute(string sql, vector<Param> &bind) {
        return 0;
    }

    // TODO
    void DB2Handle::initAsync() {
    }

    // TODO
    bool DB2Handle::isBusy() {
        return false;
    }

    // TODO
    bool DB2Handle::cancel() {
        return false;
    }

    // TODO
    bool DB2Handle::close() {
        return false;
    }
};

using namespace dbi;
using namespace std;

extern "C" {
    DB2Handle* dbdConnect(string user, string pass, string dbname, string host, string port) {
        if (host == "")
            return new DB2Handle(user, pass, dbname);
        else {
            if (port == "") port = "50000";
            return new DB2Handle(user, pass, dbname, host, port);
        }
    }

    Driver* dbdInfo(void) {
        Driver *driver  = new Driver;
        driver->name    = DRIVER_NAME;
        driver->version = DRIVER_VERSION;
        return driver;
    }
}
