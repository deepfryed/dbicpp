#include "dbic++.h"
#include "sqlcli1.h"

#define DRIVER_NAME     "db2"
#define DRIVER_VERSION  "1.3"


namespace dbi {

    using namespace std;

    #define CHECK_SUCCESS(rc, type, handle) \
        if (rc != SQL_SUCCESS) { \
            db2error(type, handle); \
            exit(0); \
        } \

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

    class DB2Result : public AbstractResult {
        protected:
        uint32_t _rowno, _rows;
        vector<ResultRow> results;
        vector<string>    _rsfields;
        vector<int>       _rstypes;
        SQLHANDLE stmt;
        SQLSMALLINT _columns;

        public:
        DB2Result();
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
        ~DB2Result();
    };

    class DB2Statement : public DB2Result {
        protected:
        string sql;
        SQLHANDLE handle;

        public:
        DB2Statement(SQLHANDLE, string);
        ~DB2Statement();
        uint32_t execute();
        uint32_t execute(vector<Param> &bind);
        void cleanup();
    };

    class DB2Handle : public AbstractHandle {
        protected:
        SQLHANDLE env, handle;
        DB2Statement *stmt;
        void setup();
        void TCPIPConnect(string, string, string, string, string);

        public:
        DB2Handle();
        DB2Handle(string, string, string);
        DB2Handle(string, string, string, string, string);
        ~DB2Handle();
        uint32_t execute(string);
        uint32_t execute(string, vector<Param> &);
        AbstractStatement* prepare(string);
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

        // ASYNC
        AbstractResult* results();
        int socket();
        AbstractResult* aexecute(string, vector<Param> &);
        void initAsync();
        bool isBusy();
        bool cancel();
    };


    // -----------------------------------------------------------------------------
    // DB2Result
    // -----------------------------------------------------------------------------

    DB2Result::DB2Result() {
        _rowno = _rows = 0;
    };

    uint32_t DB2Result::rows() {
        return _rows;
    }

    uint32_t DB2Result::columns() {
        return _columns;
    }

    vector<string> DB2Result::fields() {
        return _rsfields;
    }

    bool DB2Result::read(ResultRow &r) {
        if (_rowno < _rows) {
            r = results[_rowno++];
            return true;
        }
        return false;
    }

    // TODO
    bool DB2Result::read(ResultRowHash &r) {
        return false;
    }

    // TODO
    unsigned char* DB2Result::read(uint32_t r, uint32_t c, uint64_t *l) {
        return 0;
    }

    // TODO
    bool DB2Result::finish() {
        return false;
    }

    uint32_t DB2Result::tell() {
        return _rowno;
    }

    void DB2Result::seek(uint32_t r) {
        _rowno = r >= 0 && r < _rows ? r : _rowno;
    }

    void DB2Result::rewind() {
        _rowno = 0;
    }

    vector<int>& DB2Result::types() {
        return _rstypes;
    }

    bool DB2Result::consumeResult() {
        ResultRow r;
        SQLINTEGER length;
        char databuffer[8192];
        while (SQLFetch(stmt) != SQL_NO_DATA_FOUND) {
            r.clear();
            for (int i = 1; i <= _columns; i++ ) {
                // TODO BLOB
                // TODO refetch if length > 8k
                SQLGetData(stmt, i, SQL_C_CHAR, databuffer, 8192, &length);
                r.push_back(PARAM(databuffer));
            }
            results.push_back(r);
            _rows++;
        }
        return true;
    }

    void DB2Result::prepareResult() {
        _rows = _rowno = 0;
        results.clear();
    }

    void DB2Result::cleanup() {
        if (stmt)
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        stmt = 0;
    }

    DB2Result::~DB2Result() {
        cleanup();
    }

    // TODO
    uint64_t DB2Result::lastInsertID() {
        return 0;
    }


    // -----------------------------------------------------------------------------
    // DB2Statement
    // -----------------------------------------------------------------------------

    DB2Statement::DB2Statement(SQLHANDLE handle, string sql) : DB2Result() {
        this->handle = handle;
        this->sql    = sql;
        SQLAllocHandle(SQL_HANDLE_STMT, handle, &stmt);
    }

    uint32_t DB2Statement::execute() {
        prepareResult();
        int rc = SQLExecDirect(stmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
        CHECK_SUCCESS(rc, SQL_HANDLE_STMT, stmt);
        SQLNumResultCols(stmt, &_columns);
        consumeResult();
        return rows();
    }

    // TODO
    uint32_t DB2Statement::execute(vector<Param> &bind) {
        return 0;
    }

    void DB2Statement::cleanup() {
        if (stmt)
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        stmt = 0;
    }

    DB2Statement::~DB2Statement() {
        cleanup();
    }


    // -----------------------------------------------------------------------------
    // DB2Statement
    // -----------------------------------------------------------------------------

    DB2Handle::DB2Handle() { }

    void DB2Handle::setup() {
        // TODO error checks.
        stmt = 0;
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
        CHECK_SUCCESS(rc, SQL_HANDLE_DBC, handle);
    }

    DB2Handle::DB2Handle(string user, string pass, string dbname, string host, string port) {
        setup();
        TCPIPConnect(user, pass, dbname, host, port);
    }

    AbstractStatement* DB2Handle::prepare(string sql) {
        return (AbstractStatement*) new DB2Statement(handle, sql);
    }

    uint32_t DB2Handle::execute(string sql) {
        if (stmt) delete stmt;
        stmt = (DB2Statement*) prepare(sql);
        return stmt->execute();
    }

    // TODO
    uint32_t DB2Handle::execute(string sql, vector<Param> &bind) {
        if (stmt) delete stmt;
        stmt = (DB2Statement*) prepare(sql);
        return stmt->execute(bind);
    }

    void DB2Handle::cleanup() {
        if (stmt) delete stmt;
        stmt = 0;
        SQLDisconnect(handle);
        SQLFreeHandle(SQL_HANDLE_DBC, handle);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
    }

    DB2Handle::~DB2Handle() {
        cleanup();
    }

    // TODO
    string DB2Handle::escape(string value) {
        return "";
    }

    // TODO
    uint64_t DB2Handle::write(string table, FieldSet &fields, IO* io) {
        return 0;
    }

    // TODO
    bool DB2Handle::begin() {
    }

    // TODO
    bool DB2Handle::rollback() {
    }

    // TODO
    bool DB2Handle::commit() {
    }

    // TODO
    bool DB2Handle::begin(string name) {
    }

    // TODO
    bool DB2Handle::rollback(string name) {
    }

    // TODO
    bool DB2Handle::commit(string name) {
    }

    // TODO
    void* DB2Handle::call(string name, void* args, uint64_t len) {
        return 0;
    }

    // TODO
    void DB2Handle::reconnect() {
    }

    // TODO
    void DB2Handle::setTimeZoneOffset(int hour, int min) {
    }

    // TODO
    void DB2Handle::setTimeZone(char *name) {
    }


    AbstractResult* DB2Handle::results() {
        return 0;
    }

    int DB2Handle::socket() {
        return 0;
    }

    AbstractResult* DB2Handle::aexecute(string sql, vector<Param> &bind) {
        return 0;
    }

    void DB2Handle::initAsync() {
    }

    bool DB2Handle::isBusy() {
        return false;
    }

    bool DB2Handle::cancel() {
        return false;
    }


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
