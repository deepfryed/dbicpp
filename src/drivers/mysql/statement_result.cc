#include "common.h"

namespace dbi {
    MySqlStatementResult::MySqlStatementResult(MYSQL_STMT *stmt, MYSQL_BIND *bind, string sql) {
        _rowno         = 0;
        _rows          = mysql_stmt_num_rows(stmt);
        _cols          = mysql_stmt_field_count(stmt);
        _affected_rows = mysql_stmt_affected_rows(stmt);

        _sql           = sql;
        resultset      = 0;
        last_insert_id = mysql_stmt_insert_id(stmt);

        if (bind) {
            fetchMeta(stmt);
            storeResult(stmt, bind);
        }
    }

    uint32_t MySqlStatementResult::rows() {
        return _affected_rows > 0 ? _affected_rows : _rows;
    }

    uint32_t MySqlStatementResult::columns() {
        return _cols;
    }

    bool MySqlStatementResult::read(ResultRow &row) {
        if (!resultset) return false;

        row.clear();
        if (_rowno < _rows) {
            _rowno++;
            struct rowdata tuple = resultset[_rowno-1];
            for (int n = 0; n < _cols; n++)
                row.push_back(tuple.data[n] == 0 ? PARAM(null()) : PARAM((unsigned char*)tuple.data[n], tuple.len[n]));
            return true;
        }

        return false;
    }

    bool MySqlStatementResult::read(ResultRowHash &rowhash) {
        if (!resultset) return false;

        rowhash.clear();

        if (_rowno < _rows) {
            _rowno++;
            struct rowdata tuple = resultset[_rowno-1];
            for (int n = 0; n < _cols; n++) {
                rowhash[_rsfields[n]] = tuple.data[n] == 0 ? PARAM(null()) :
                                                             PARAM((unsigned char*)tuple.data[n], tuple.len[n]);
            }
            return true;
        }

        return false;
    }

    unsigned char* MySqlStatementResult::read(uint32_t r, uint32_t c, uint64_t *l) {
        if (resultset && r < _rows) {
            if (l) *l = resultset[r].len[c];
            return resultset[r].data[c];
        }
        return 0;
    }

    bool MySqlStatementResult::finish() {
        return true;
    }

    void MySqlStatementResult::seek(uint32_t r) {
        _rowno = r;
    }

    uint32_t MySqlStatementResult::tell() {
        return _rowno;
    }

    uint64_t MySqlStatementResult::lastInsertID() {
        return last_insert_id;
    }

    void MySqlStatementResult::checkReady(string m) {
        // NOP
    }

    bool MySqlStatementResult::consumeResult() {
        // NOP
        return false;
    }

    void MySqlStatementResult::prepareResult() {
        // NOP
    }

    void MySqlStatementResult::cleanup() {
        if (resultset) {
            for (int r = 0; r < _rows; r++) {
                if (resultset[r].data) {
                    for (int c = 0; c < _cols; c++) {
                        if (resultset[r].data[c]) free(resultset[r].data[c]);
                    }
                    free(resultset[r].data);
                }
                if (resultset[r].len) free(resultset[r].len);
            }
            free(resultset);
        }
        resultset = 0;
    }

    void MySqlStatementResult::storeResult(MYSQL_STMT *stmt, MYSQL_BIND *bind) {
        resultset = (struct rowdata *)malloc(sizeof(struct rowdata)*_rows);
        bzero(resultset, sizeof(struct rowdata) * _rows);

        mysql_stmt_row_seek(stmt, stmt->result.data);

        int r, c, rc;
        for (r = 0; r < _rows; r++) {
            resultset[r].data = (unsigned char**)malloc(sizeof(unsigned char*) * _cols);
            resultset[r].len  = (unsigned long*)malloc(sizeof(unsigned long) * _cols);

            bzero(resultset[r].data, sizeof(unsigned char*) * _cols);

            rc = mysql_stmt_fetch(stmt);

            if (rc != 0 && rc != MYSQL_DATA_TRUNCATED) {
                cleanup();
                throw RuntimeError(mysql_stmt_error(stmt));
            }

            for (c = 0; c < _cols; c++) {
                if (*bind[c].is_null) continue;

                if (bind[c].buffer_length < *bind[c].length) {
                    delete [] (unsigned char*)bind[c].buffer;
                    bind[c].buffer = (void*)new unsigned char[*bind[c].length];
                    bind[c].buffer_length = *bind[c].length;
                    mysql_stmt_fetch_column(stmt, &bind[c], c, 0);
                }

                resultset[r].len[c]  = *bind[c].length;
                resultset[r].data[c] = (unsigned char*)malloc(*bind[c].length);
                memcpy(resultset[r].data[c], bind[c].buffer, *bind[c].length);
            }
        }
    }

    void MySqlStatementResult::fetchMeta(MYSQL_STMT *stmt) {
        int n;
        MYSQL_FIELD *fields;
        MYSQL_RES *result = mysql_stmt_result_metadata(stmt);

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

        mysql_free_result(result);
    }

    void MySqlStatementResult::rewind() {
        _rowno = 0;
    }

    vector<string>& MySqlStatementResult::fields() {
        return _rsfields;
    }

    vector<int>& MySqlStatementResult::types() {
        return _rstypes;
    }
}
