#include "common.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace dbi {
    void MySqlBinaryResult::set_zero_time(MYSQL_TIME *tm, enum enum_mysql_timestamp_type time_type) {
        bzero((void*) tm, sizeof(*tm));
        tm->time_type = time_type;
    }

    unsigned long MySqlBinaryResult::net_field_length(unsigned char **packet) {
        unsigned char *pos= (unsigned char *)*packet;
        if (*pos < 251) {
            (*packet)++;
            return (unsigned long) *pos;
        }

        if (*pos == 251) {
            (*packet)++;
            return NULL_LENGTH;
        }

        if (*pos == 252) {
            (*packet) +=3;
            return (unsigned long) uint2korr(pos+1);
        }

        if (*pos == 253) {
            (*packet) +=4;
            return (unsigned long) uint3korr(pos+1);
        }

        (*packet) +=9;                 /* Must be 254 when here */
        return (unsigned long) uint4korr(pos+1);
    }

    void MySqlBinaryResult::read_binary_time(MYSQL_TIME* tm, unsigned char **pos) {
        /* net_field_length will set pos to the first byte of data */
        unsigned int length = net_field_length(pos);

        if (length) {
            unsigned char *to = *pos;

            tm->neg         = to[0];
            tm->day         = (unsigned long) sint4korr(to+1);
            tm->hour        = (unsigned int) to[5];
            tm->minute      = (unsigned int) to[6];
            tm->second      = (unsigned int) to[7];
            tm->second_part = (length > 8) ? (unsigned long) sint4korr(to+8) : 0;

            tm->year = tm->month= 0;
            if (tm->day) {
                /* Convert days to hours at once */
                tm->hour += tm->day*24;
                tm->day = 0;
            }
            tm->time_type = MYSQL_TIMESTAMP_TIME;

            *pos += length;
        }
        else
            set_zero_time(tm, MYSQL_TIMESTAMP_TIME);
    }

    void MySqlBinaryResult::read_binary_datetime(MYSQL_TIME *tm, unsigned char **pos) {
        unsigned int length = net_field_length(pos);

        if (length) {
            unsigned char *to = *pos;

            tm->neg   = 0;
            tm->year  = (unsigned int) sint2korr(to);
            tm->month = (unsigned int) to[2];
            tm->day   = (unsigned int) to[3];

            if (length > 4) {
                tm->hour   = (unsigned int) to[4];
                tm->minute = (unsigned int) to[5];
                tm->second = (unsigned int) to[6];
            }
            else {
                tm->hour = tm->minute= tm->second= 0;
            }

            tm->second_part = (length > 7) ? (unsigned long) sint4korr(to+7) : 0;
            tm->time_type   = MYSQL_TIMESTAMP_DATETIME;

            *pos += length;
        }
        else
          set_zero_time(tm, MYSQL_TIMESTAMP_DATETIME);
    }

    void MySqlBinaryResult::read_binary_date(MYSQL_TIME *tm, unsigned char **pos) {
        unsigned int length= net_field_length(pos);

        if (length) {
            unsigned char *to = *pos;

            tm->year  =  (unsigned int) sint2korr(to);
            tm->month =  (unsigned int) to[2];
            tm->day   =  (unsigned int) to[3];

            tm->hour        = tm->minute= tm->second= 0;
            tm->second_part = 0;
            tm->neg         = 0;
            tm->time_type   = MYSQL_TIMESTAMP_DATE;

            *pos += length;
        }
        else
            set_zero_time(tm, MYSQL_TIMESTAMP_DATE);
    }

    void MySqlBinaryResult::fetch_result_tinyint(int c, unsigned char **row) {
        pool[c].length = 0;
        snprintf((char*)pool[c].data, 5, "%d", **row);
        (*row)++;
    }

    void MySqlBinaryResult::fetch_result_short(int c, unsigned char **row) {
        pool[c].length = 0;
        snprintf((char*)pool[c].data, 8, "%d", (int16_t) sint2korr(*row));
        *row+= 2;
    }

    void MySqlBinaryResult::fetch_result_int32(int c, unsigned char **row) {
        pool[c].length = 0;
        snprintf((char*)pool[c].data, 16, "%d", (int32_t) sint4korr(*row));
        *row+= 4;
    }

    void MySqlBinaryResult::fetch_result_int64(int c, unsigned char **row) {
        pool[c].length = 0;
        snprintf((char*)pool[c].data, 32, "%" PRIi64 , (int64_t) sint8korr(*row));
        *row+= 8;
    }

    void MySqlBinaryResult::fetch_result_float(int c, unsigned char **row) {
        float value;
        pool[c].length = 0;
        float4get(value, *row);
        snprintf((char*)pool[c].data, 32, "%f", value);
        *row+= 4;
    }

    void MySqlBinaryResult::fetch_result_double(int c, unsigned char **row) {
        double value;
        pool[c].length = 0;
        float8get(value, *row);
        snprintf((char*)pool[c].data, 64, "%lf", value);
        *row+= 8;
    }

    void MySqlBinaryResult::fetch_result_time(int c, unsigned char **row) {
        MYSQL_TIME tm;
        pool[c].length = 0;
        read_binary_time(&tm, row);
        snprintf((char*)pool[c].data, 128, "%02d:%02d:%02d", tm.hour, tm.minute, tm.second);
    }

    void MySqlBinaryResult::fetch_result_date(int c, unsigned char **row) {
        MYSQL_TIME tm;
        pool[c].length = 0;
        read_binary_date(&tm, row);
        snprintf((char*)pool[c].data, 128, "%04d-%02d-%02d", tm.year, tm.month, tm.day);
    }

    void MySqlBinaryResult::fetch_result_datetime(int c, unsigned char **row) {
        MYSQL_TIME tm;
        pool[c].length = 0;
        read_binary_datetime(&tm, row);
        snprintf((char*)pool[c].data, 128, "%04d-%02d-%02d %02d:%02d:%02d", tm.year, tm.month, tm.day,
                                                                            tm.hour, tm.minute, tm.second);
    }

    void MySqlBinaryResult::fetch_result_bin(int c, unsigned char **row) {
        unsigned long length = net_field_length(row);
        if (pool[c].alloc < length + 1) {
            delete [] pool[c].data;
            pool[c].data  = new unsigned char[length+1];
            pool[c].alloc = length + 1;
        }

        memcpy(pool[c].data, (unsigned char *)*row, length);

        pool[c].length       = length;
        pool[c].data[length] = '\0';

        *row+= length;
    }

    void MySqlBinaryResult::fetchMeta() {
        int n;
        MYSQL_FIELD *fields;

        _cols  = mysql_num_fields(result);
        _rows  = mysql_num_rows(result);

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

    void MySqlBinaryResult::fetchRow() {
        if (!cursor) return;

        unsigned char *nulls, *row = (unsigned char*)cursor->data, bit = 4;

        nulls = row;
        row  += (_cols + 9) / 8;
        // 2 reserved bits and 1 bit for each column to denote null value (byte aligned)

        for (int j = 0; j < _cols; j++) {
            if (*nulls & bit) {
                pool[j].isnull = true;
            }
            else {
                pool[j].isnull = false;
                switch(result->fields[j].type) {
                    case MYSQL_TYPE_TINY:
                        fetch_result_tinyint(j, &row);
                        break;
                    case MYSQL_TYPE_SHORT:
                        fetch_result_short(j, &row);
                        break;
                    case MYSQL_TYPE_LONG:
                        fetch_result_int32(j, &row);
                        break;
                    case MYSQL_TYPE_LONGLONG:
                        fetch_result_int64(j, &row);
                        break;
                    case MYSQL_TYPE_FLOAT:
                        fetch_result_float(j, &row);
                        break;
                    case MYSQL_TYPE_DOUBLE:
                        fetch_result_double(j, &row);
                        break;
                    case MYSQL_TYPE_TIMESTAMP:
                    case MYSQL_TYPE_DATETIME:
                        fetch_result_datetime(j, &row);
                        break;
                    case MYSQL_TYPE_TIME:
                        fetch_result_time(j, &row);
                        break;
                    case MYSQL_TYPE_DATE:
                        fetch_result_date(j, &row);
                        break;
                    /*
                    TODO: check string and blob types and throw an error in default.
                    case MYSQL_TYPE_TINY_BLOB:
                    case MYSQL_TYPE_MEDIUM_BLOB:
                    case MYSQL_TYPE_LONG_BLOB:
                    case MYSQL_TYPE_BLOB:
                    case MYSQL_TYPE_BIT:
                        data = fetch_result_bin(&row, &length);
                        break;
                    */
                    default:
                        fetch_result_bin(j, &row);
                        break;
                }
            }

            if (!((bit<<=1) & 255)) {
                bit = 1;
                nulls++;
            }
        }

        cursor = cursor->next;
    }

    bool MySqlBinaryResult::read(ResultRow &row) {
        if (_rowno >= _rows) return false;

        fetchRow();
        _rowno++;
        row.resize(_cols);

        for (int n = 0; n < _cols; n++) {
            if (pool[n].isnull) {
                row[n].isnull = true;
                row[n].value  = "";
            }
            else {
                row[n].isnull = false;
                row[n].value  = pool[n].length > 0 ? string((char*)pool[n].data, pool[n].length)
                                                   : string((char*)pool[n].data);
                row[n].binary = _rstypes[n] == DBI_TYPE_BLOB;
            }
        }
        return true;
    }

    bool MySqlBinaryResult::read(ResultRowHash &rowhash) {
        if (_rowno >= _rows) return false;
        fetchRow();

        _rowno++;
        rowhash.clear();
        for (int n = 0; n < _cols; n++) {
            rowhash[_rsfields[n]] = pool[n].isnull ?
                PARAM(null()) :
                pool[n].length > 0 ? PARAM(pool[n].data, pool[n].length) : PARAM((char*)pool[n].data);
        }
        return true;
    }

    unsigned char* MySqlBinaryResult::read(uint32_t r, uint32_t c, uint64_t *length) {
        if (r >= _rows || c >= _cols || r < 0 || c < 0) return 0;

        // if row data is not already buffered
        if (r+1 != _rowno) {
            // need first row
            if (r == 0) rewind();
            // or somewhere in the middle (r = _rowno is the next row to one already buffered).
            else if (r != _rowno) seek(r);

            fetchRow();
            _rowno = r + 1;
        }

        if (length) *length = pool[c].isnull ? 0 : pool[c].length > 0 ? pool[c].length : strlen((char*)pool[c].data);
        return pool[c].isnull ? 0 : (unsigned char*)pool[c].data;
    }

    uint32_t MySqlBinaryResult::rows() {
        return _affected_rows > 0 ? _affected_rows : _rows;
    }

    uint32_t MySqlBinaryResult::columns() {
        return _cols;
    }

    uint64_t MySqlBinaryResult::lastInsertID() {
        return last_insert_id;
    }

    vector<string>& MySqlBinaryResult::fields() {
        return _rsfields;
    }

    vector<int>& MySqlBinaryResult::types() {
        return _rstypes;
    }

    uint32_t MySqlBinaryResult::tell() {
        return _rowno;
    }

    void MySqlBinaryResult::seek(uint32_t n) {
        if (n >= _rows) return;

        _rowno = n;
        cursor = mysqldata;
        while (cursor && n--) {
            cursor = cursor->next;
        }
    }

    void MySqlBinaryResult::rewind() {
        cursor = mysqldata;
        _rowno = 0;
    }

    MySqlBinaryResult::MySqlBinaryResult(MYSQL_STMT *stmt) {
        pool   = 0;
        _rows  = 0;
        _cols  = 0;
        _rowno = 0;

        _affected_rows = POSITIVE_OR_ZERO(mysql_stmt_affected_rows(stmt));
        last_insert_id = mysql_stmt_insert_id(stmt);

        if ((result = mysql_stmt_result_metadata(stmt))) {
            result->row_count = mysql_stmt_num_rows(stmt);
            fetchMeta();
        }

        // make a copy of the binary resultset.
        mysqldata = cursor = 0;
        if (_rows > 0) {
            MYSQL_ROWS *next, *curr = stmt->result.data;
            while (curr) {
                next = new MYSQL_ROWS;
                next->next   = 0;
                next->length = curr->length;
                next->data   = (char**)new unsigned char[curr->length];
                memcpy(next->data, curr->data, curr->length);

                if (cursor) {
                    cursor->next = next;
                    cursor = next;
                }
                else {
                    mysqldata = cursor = next;
                }

                curr = curr->next;
            }

            cursor = mysqldata;

            if (_cols > 0) pool = new ResultBuffer[_cols];
            for (int n = 0; n < _cols; n++) {
                pool[n].data   = new unsigned char[1024];
                pool[n].alloc  = 1024;
                pool[n].length = 0;
            }
        }
    }

    MySqlBinaryResult::~MySqlBinaryResult() {
        cleanup();
    }

    void MySqlBinaryResult::cleanup() {
        if (result) {
            mysql_free_result(result);
            result = 0;
        }

        if (mysqldata) {
            cursor = mysqldata;
            MYSQL_ROWS *next;
            while (cursor) {
                next = cursor->next;
                if (cursor->length && cursor->data) delete [] (unsigned char*)cursor->data;
                delete cursor;
                cursor = next;
            }
            mysqldata = 0;
        }

        if (pool) {
            for (int n = 0; n < _cols; n++) delete [] pool[n].data;
            delete [] pool;
            pool = 0;
        }
    }

    // NOP

    bool MySqlBinaryResult::finish() {
        return true;
    }

    bool MySqlBinaryResult::consumeResult() {
        return true;
    }

    void MySqlBinaryResult::prepareResult() {
    }
}
