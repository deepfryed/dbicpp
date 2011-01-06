#include "common.h"

namespace dbi {
    MySqlResult::MySqlResult(MYSQL_RES *r, string sql, MYSQL *c) {
        _rows          = 0;
        _cols          = 0;
        _rowno         = 0;
        _affected_rows = 0;

        conn   = c;
        result = r;

        if (conn) {
            last_insert_id = mysql_insert_id(conn);
            _affected_rows = POSITIVE_OR_ZERO(mysql_affected_rows(conn));
        }

        if (result) fetchMeta(result);
    }

    MySqlResult::~MySqlResult() {
        cleanup();
    }

    uint32_t MySqlResult::rows() {
        return _affected_rows > 0 ? _affected_rows : _rows;
    }

    uint32_t MySqlResult::columns() {
        return _cols;
    }

    bool MySqlResult::read(ResultRow &row) {
        if (!result) return false;

        if (_rowno < _rows) {
            _rowno++;
            row.resize(_cols);

            _rowdata         = mysql_fetch_row(result);
            _rowdata_lengths = mysql_fetch_lengths(result);
            for (int n = 0; n < _cols; n++) {
                if(_rowdata[n] == 0) {
                    row[n].isnull = true;
                    row[n].value  = "";
                }
                else {
                    row[n].isnull = false;
                    row[n].value  = string(_rowdata[n], _rowdata_lengths[n]);
                    row[n].binary = _rstypes[n] == DBI_TYPE_BLOB;
                }
            }
            return true;
        }

        return false;
    }

    bool MySqlResult::read(ResultRowHash &rowhash) {
        if (!result) return false;

        if (_rowno < _rows) {
            _rowno++;
            rowhash.clear();

            _rowdata         = mysql_fetch_row(result);
            _rowdata_lengths = mysql_fetch_lengths(result);

            for (int n = 0; n < _cols; n++)
                rowhash[_rsfields[n]] = (
                    _rowdata[n] == 0 ? PARAM(null()) : PARAM((unsigned char*)_rowdata[n], _rowdata_lengths[n])
                );
            return true;
        }

        return false;
    }

    unsigned char* MySqlResult::read(uint32_t r, uint32_t c, uint64_t *l) {
        if (r >= _rows || c >= _cols || r < 0 || c < 0) return 0;

        // if row data is not already buffered
        if (r+1 != _rowno) {
            // need first row
            if (r == 0) rewind();
            // or somewhere in the middle (r = _rowno is the next row to one already buffered).
            else if (r != _rowno) seek(r);

            _rowdata         = mysql_fetch_row(result);
            _rowdata_lengths = mysql_fetch_lengths(result);
            _rowno = r + 1;
        }

        if (l) *l = _rowdata_lengths[c];
        return (unsigned char*)_rowdata[c];
    }

    bool MySqlResult::finish() {
        if (result) mysql_free_result(result);
        result = 0;
        return true;
    }

    void MySqlResult::seek(uint32_t r) {
        mysql_data_seek(result, r);
        _rowno = r;
    }

    uint32_t MySqlResult::tell() {
        return _rowno;
    }

    void MySqlResult::cleanup() {
        finish();
    }

    uint64_t MySqlResult::lastInsertID() {
        return last_insert_id;
    }

    void MySqlResult::lastInsertID(uint64_t value) {
        last_insert_id = value;
    }

    void MySqlResult::checkReady(string m) {
        // NOP
    }

    // TODO Handle reconnects in consumeResult() and prepareResult()
    bool MySqlResult::consumeResult() {
        if (conn && mysql_read_query_result(conn) != 0) {
            finish();
            throw RuntimeError(mysql_error(conn));
        }
        return false;
    }

    void MySqlResult::prepareResult() {
        if (conn) {
            result = mysql_store_result(conn);
            fetchMeta(result);
        }

        last_insert_id = mysql_insert_id(conn);
        _affected_rows = POSITIVE_OR_ZERO(mysql_affected_rows(conn));
    }

    void MySqlResult::fetchMeta(MYSQL_RES* result) {
        int n;
        MYSQL_FIELD *fields;

        _rows  = mysql_num_rows(result);
        _cols  = mysql_num_fields(result);

        fields = mysql_fetch_fields(result);
        for (n = 0; n < _cols; n++) {
            _rsfields.push_back(fields[n].name);
            switch(fields[n].type) {
                case MYSQL_TYPE_TINY:
                    _rstypes.push_back(fields[n].length == 1 ? DBI_TYPE_BOOLEAN : DBI_TYPE_INT);
                    break;
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_INT24:
                case MYSQL_TYPE_LONGLONG:
                case MYSQL_TYPE_YEAR:
                    _rstypes.push_back(DBI_TYPE_INT);
                    break;
                case MYSQL_TYPE_DECIMAL:
                case MYSQL_TYPE_NEWDECIMAL:
                    _rstypes.push_back(DBI_TYPE_NUMERIC);
                    break;
                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE:
                    _rstypes.push_back(DBI_TYPE_FLOAT);
                    break;
                case MYSQL_TYPE_TIMESTAMP:
                case MYSQL_TYPE_DATETIME:
                    _rstypes.push_back(DBI_TYPE_TIMESTAMP);
                    break;
                case MYSQL_TYPE_TIME:
                    _rstypes.push_back(DBI_TYPE_TIME);
                    break;
                case MYSQL_TYPE_DATE:
                    _rstypes.push_back(DBI_TYPE_DATE);
                    break;
                default:
                    _rstypes.push_back((fields[n].flags & BINARY_FLAG) ? DBI_TYPE_BLOB : DBI_TYPE_TEXT);
                    break;
            }
        }
    }

    void MySqlResult::rewind() {
        _rowno = 0;
        mysql_data_seek(result, 0);
    }

    vector<string>& MySqlResult::fields() {
        return _rsfields;
    }

    vector<int>& MySqlResult::types() {
        return _rstypes;
    }
}
