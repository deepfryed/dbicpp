#include "common.h"

namespace dbi {
    void PgResult::init() {
        _result        = 0;
        _rowno         = 0;
        _rows          = 0;
        _cols          = 0;
        _bytea         = 0;
        _affected_rows = 0;
    }

    PgResult::~PgResult() {
        cleanup();
    }

    PgResult::PgResult(PGresult *r, string sql, PGconn *c) {
        init();

        _result = r;
        _conn   = c;
        _sql    = sql;

        if (_result) fetchMeta();
    }

    void PgResult::fetchMeta() {
        _rows          = PQntuples(_result);
        _cols          = PQnfields(_result);
        _affected_rows = (uint32_t)atoi(PQcmdTuples(_result));

        for (int i = 0; i < (int)_cols; i++)
            _rsfields.push_back(PQfname(_result, i));

        // postgres types are all in pg_type. no clue why they're not in the headers.
        for (int i = 0; i < (int)_cols; i++) {
            switch(PQftype(_result, i)) {
                case   16: _rstypes.push_back(DBI_TYPE_BOOLEAN); break;
                case   17: _rstypes.push_back(DBI_TYPE_BLOB); break;
                case   20:
                case   21:
                case   23: _rstypes.push_back(DBI_TYPE_INT); break;
                case   25: _rstypes.push_back(DBI_TYPE_TEXT); break;
                case  700:
                case  701: _rstypes.push_back(DBI_TYPE_FLOAT); break;
                case 1082: _rstypes.push_back(DBI_TYPE_DATE); break;
                case 1114:
                case 1184: _rstypes.push_back(DBI_TYPE_TIMESTAMP); break;
                case 1700: _rstypes.push_back(DBI_TYPE_NUMERIC); break;
                case 1083:
                case 1266: _rstypes.push_back(DBI_TYPE_TIME); break;
                  default: _rstypes.push_back(DBI_TYPE_TEXT); break;
            }
        }
    }

    uint32_t PgResult::rows() {
        return _affected_rows > 0 ? _affected_rows : _rows;
    }

    bool PgResult::consumeResult() {
        PQconsumeInput(_conn);
        return (PQisBusy(_conn) ? true : false);
    }

    void PgResult::prepareResult() {
        PGresult *response;
        _result = PQgetResult(_conn);
        fetchMeta();

        while ((response = PQgetResult(_conn))) PQclear(response);
        PQ_CHECK_RESULT(_result, _conn, _sql);
    }

    uint64_t PgResult::lastInsertID() {
        return PQgetisnull(_result, 0, 0) ? 0 : atol(PQgetvalue(_result, 0, 0));
    }

    unsigned char* PgResult::unescapeBytea(int r, int c, uint64_t *l) {
        size_t len;
        if (_bytea) PQfreemem(_bytea);
        _bytea = PQunescapeBytea((unsigned char *)PQgetvalue(_result, r, c), &len);
        *l = (uint64_t)len;
        return _bytea;
    }

    bool PgResult::read(ResultRow &row) {
        uint64_t len;
        unsigned char *data;

        if (_rowno < _rows) {
            row.resize(_cols);

            for (int n = 0; n < _cols; n++) {
                if (PQgetisnull(_result, _rowno, n)) {
                    row[n].isnull = true;
                    row[n].value  = "";
                }
                else if (_rstypes[n] != DBI_TYPE_BLOB) {
                    row[n].isnull = false;
                    row[n].binary = false;
                    row[n].value  = string(PQgetvalue(_result, _rowno, n), PQgetlength(_result, _rowno, n));
                }
                else {
                    data = unescapeBytea(_rowno, n, &len);
                    row[n].isnull = false;
                    row[n].binary = true;
                    row[n].value  = string((char*)data, len);
                }
            }
            _rowno++;
            return true;
        }

        return false;
    }

    bool PgResult::read(ResultRowHash &rowhash) {
        uint64_t len;
        unsigned char *data;

        rowhash.clear();

        if (_rowno < _rows) {
            for (uint32_t i = 0; i < _cols; i++) {
                if (PQgetisnull(_result, _rowno, i))
                    rowhash[_rsfields[i]] = PARAM(null());
                else if (_rstypes[i] != DBI_TYPE_BLOB)
                    rowhash[_rsfields[i]] = PG2PARAM(_result, _rowno, i);
                else {
                    data = unescapeBytea(_rowno, i, &len);
                    rowhash[_rsfields[i]] = PARAM(data, len);
                }
            }
            _rowno++;
            return true;
        }

        return false;
    }

    string_list_t& PgResult::fields() {
        return _rsfields;
    }

    uint32_t PgResult::columns() {
        return _cols;
    }

    void PgResult::cleanup() {
        if (_bytea)  PQfreemem(_bytea);
        if (_result) PQclear(_result);

        _rsfields.clear();
        _rstypes.clear();

        _rowno  = 0;
        _rows   = 0;
        _result = 0;
        _bytea  = 0;
    }

    unsigned char* PgResult::read(uint32_t r, uint32_t c, uint64_t *l) {
        if (r >= _rows || c >= _cols || r < 0 || c < 0) return 0;

        _rowno = r;
        if (PQgetisnull(_result, r, c)) return 0;

        if (_rstypes[c] != DBI_TYPE_BLOB) {
            if (l) *l = PQgetlength(_result, r, c);
            return (unsigned char*)PQgetvalue(_result, r, c);
        }
        else return unescapeBytea(r, c, l);
    }

    uint32_t PgResult::tell() {
        return _rowno;
    }

    void PgResult::boom(const char* m) {
        if (PQstatus(_conn) == CONNECTION_BAD)
            throw ConnectionError(m);
        else
            throw RuntimeError(m);
    }

    void PgResult::rewind() {
        _rowno = 0;
    }

    void PgResult::seek(uint32_t r) {
        _rowno = r;
    }

    int_list_t& PgResult::types() {
        return _rstypes;
    }
}
