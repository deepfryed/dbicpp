#include "common.h"

#define CSTRING_OR_NULL(s) (s.size() > 0 ? s.c_str() : 0)

namespace dbi {
    MySqlHandle::MySqlHandle() {
        tr_nesting = 0;
    }

    MySqlHandle::MySqlHandle(string user, string pass, string dbname, string host, string port, char *options) {
        tr_nesting   = 0;
        _result      = 0;

        uint32_t _port = atoi(port.c_str());

        conn  = mysql_init(0);
        _db   = dbname;
        _host = host;

        mysql_options(conn, MYSQL_OPT_RECONNECT,    &MYSQL_BOOL_TRUE);
        mysql_options(conn, MYSQL_OPT_LOCAL_INFILE, 0);

        if (options)
            parseOptions(options);

        if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(),
            _port, 0, CLIENT_FOUND_ROWS))
            connectionError();

        mysql_set_character_set(conn, "utf8");
        mysql_set_local_infile_handler(conn,
            LOCAL_INFILE_INIT, LOCAL_INFILE_READ, LOCAL_INFILE_END, LOCAL_INFILE_ERROR, 0);
    }

    void MySqlHandle::parseOptions(char *options) {
        pcrecpp::RE re("([^ =;]+) *= *([^ =;]+)");
        pcrecpp::StringPiece input(options);

        string option;
        string value;

        bool ssl = false;
        string opt_ssl_key, opt_ssl_cert, opt_ssl_ca, opt_ssl_capath, opt_ssl_cipher;
        while (re.FindAndConsume(&input, &option, &value)) {
            if      (option == "sslkey")    { opt_ssl_key  = value; ssl = true; }
            else if (option == "sslcert")   { opt_ssl_cert = value; ssl = true; }
            else if (option == "sslca")     { opt_ssl_ca   = value; ssl = true; }
            else if (option == "sslcapath") { opt_ssl_capath = value; ssl = true; }
            else if (option == "sslcipher") { opt_ssl_cipher = value; ssl = true; }
        }

        if (ssl) {
            mysql_ssl_set(conn,
                CSTRING_OR_NULL(opt_ssl_key),
                CSTRING_OR_NULL(opt_ssl_cert),
                CSTRING_OR_NULL(opt_ssl_ca),
                CSTRING_OR_NULL(opt_ssl_capath),
                CSTRING_OR_NULL(opt_ssl_cipher));
        }
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

    string MySqlHandle::driver() {
        return DRIVER_NAME;
    }

    void MySqlHandle::cleanup() {
        if (conn) {
            if (_result) mysql_free_result(_result);
            mysql_close(conn);
        }

        _result    = 0;
        conn       = 0;
    }

    uint32_t MySqlHandle::execute(string sql) {
        int rows;

        if (_result) mysql_free_result(_result);

        _sql       = sql;
        _result    = 0;

        MYSQL_PREPROCESS_QUERY(sql);
        if (mysql_real_query(conn, sql.c_str(), sql.length()) != 0) boom(mysql_error(conn));

        rows = mysql_affected_rows(conn);
        if (rows < 0) {
            _result = mysql_store_result(conn);
            rows    = mysql_num_rows(_result);
        }

        return rows;
    }

    uint32_t MySqlHandle::execute(string sql, vector<Param> &bind) {
        int rows;

        if (_result) mysql_free_result(_result);

        _sql       = sql;
        _result    = 0;

        MYSQL_PREPROCESS_QUERY(sql);
        MYSQL_INTERPOLATE_BIND(conn, sql, bind);
        if (mysql_real_query(conn, sql.c_str(), sql.length()) != 0) boom(mysql_error(conn));

        rows = mysql_affected_rows(conn);
        if (rows < 0) {
            _result = mysql_store_result(conn);
            rows    = mysql_num_rows(_result);
        }

        return rows;
    }

    AbstractResult* MySqlHandle::result() {
        MySqlResult *rv = _result ? new MySqlResult(_result, _sql, conn) : new MySqlResult(0, _sql, conn);
        _result = 0;
        return rv;
    }

    int MySqlHandle::socket() {
        return conn->net.fd;
    }

    MySqlResult* MySqlHandle::aexecute(string sql) {
        MYSQL_PREPROCESS_QUERY(sql);
        mysql_send_query(conn, sql.c_str(), sql.length());
        return new MySqlResult(0, _sql, conn);
    }

    MySqlResult* MySqlHandle::aexecute(string sql, vector<Param> &bind) {
        MYSQL_PREPROCESS_QUERY(sql);
        MYSQL_INTERPOLATE_BIND(conn, sql, bind);
        mysql_send_query(conn, sql.c_str(), sql.length());
        return new MySqlResult(0, _sql, conn);
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
        return new MySqlStatement(sql, conn);
    }

    bool MySqlHandle::begin() {
        execute("BEGIN WORK");
        tr_nesting++;
        return true;
    };

    bool MySqlHandle::commit() {
        execute("COMMIT");
        tr_nesting = 0;
        return true;
    };

    bool MySqlHandle::rollback() {
        execute("ROLLBACK");
        tr_nesting = 0;
        return true;
    };

    bool MySqlHandle::begin(string name) {
        if (tr_nesting == 0) {
            begin();
            tr_nesting = 0;
        }
        execute("SAVEPOINT " + name);
        tr_nesting++;
        return true;
    };

    bool MySqlHandle::commit(string name) {
        execute("RELEASE SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 0)
            commit();
        return true;
    };

    bool MySqlHandle::rollback(string name) {
        execute("ROLLBACK TO SAVEPOINT " + name);
        tr_nesting--;
        if (tr_nesting == 0)
            rollback();
        return true;
    };

    bool MySqlHandle::close() {
        cleanup();
        return true;
    }

    void MySqlHandle::reconnect() {
        if (conn) {
            if (tr_nesting > 0)
                connectionError("Lost connection inside a transaction, unable to reconnect");
            if(mysql_ping(conn) != 0)
                connectionError("Lost connection, unable to reconnect");
            else {
                sprintf(errormsg, "NOTICE: Socket changed during auto reconnect to database %s on host %s\n",
                    _db.c_str(), _host.c_str());
                if (_trace)
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

    uint64_t MySqlHandle::write(string table, FieldSet &fields, IO* io) {
        char buffer[4096];
        string filename = generateCompactUUID();

        CopyInList[filename] = io;

        if (fields.size() > 0)
            snprintf(buffer, 4095, "load data local infile '%s' replace into table %s (%s)",
                filename.c_str(), table.c_str(), fields.join(", ").c_str());
        else
            snprintf(buffer, 4095, "load data local infile '%s' replace into table %s",
                filename.c_str(), table.c_str());

        if (_trace)
            logMessage(_trace_fd, buffer);

        if (mysql_real_query(conn, buffer, strlen(buffer))) {
            CopyInList.erase(filename);
            sprintf(buffer, "Error while copying data into table: %s - %s", table.c_str(), mysql_error(conn));
            boom(buffer);
        }

        return (uint64_t) mysql_affected_rows(conn);
    }

    string MySqlHandle::escape(string value) {
        char *dest = new char[value.length()*2 + 1];
        mysql_real_escape_string(conn, dest, value.data(), value.length());
        string escaped(dest);
        delete [] dest;
        return escaped;
    }
}

