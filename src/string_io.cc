#include "dbic++.h"

namespace dbi {
    StringIO::StringIO() {
        rpos   = 0;
        wpos   = 0;
        alloc  = 1024;
        data   = (unsigned char*)malloc(alloc);
        if (!data) throw RuntimeError("Out of memory: StringIO");
    }

    StringIO::StringIO(const char *v, uint64_t size) {
        rpos   = 0;
        wpos   = 0;
        alloc = size*2 + 1;
        data   = (unsigned char*)malloc(alloc);
        if (!data) throw RuntimeError("Out of memory: StringIO");
        memcpy(data+wpos, v, size);
        wpos += size;
    }

    void StringIO::write(const char *v, uint64_t size) {
        if (alloc < wpos + size) {
            alloc += size*2 + 1;
            data = (unsigned char*)realloc(data, alloc);
            if (!data) throw RuntimeError("Out of memory: StringIO");
        }
        memcpy(data+wpos, v, size);
        wpos += size;
    }

    void StringIO::write(const char *v) {
        write(v, strlen(v));
    }

    void StringIO::writef(const char *fmt, ...) {
        char buffer[65536];
        va_list ap;
        va_start(ap, fmt);
        uint64_t size = vsnprintf(buffer, 65535, fmt, ap);
        va_end(ap);
        write(buffer, size);
    }

    string& StringIO::read() {
        if (rpos < wpos) {
            char buffer[16384];
            uint64_t max = wpos - rpos, size = max > 16384 ? 16384 : max;
            memcpy(buffer, data+rpos, size);
            rpos += size;
            stringdata = string(buffer, size);
            return stringdata;
        }
        else return empty;
    }

    uint32_t StringIO::read(char *buffer, uint32_t size) {

        if (rpos < wpos) {
            uint64_t max = wpos - rpos;
            size = size > max ? max : size;
            memcpy(buffer, data+rpos, size);
            rpos += size;
            return size;
        }
        return 0;
    }

    StringIO::~StringIO() {
        if (data) free(data);
        data = 0;
        wpos = rpos = 0;
    }

    void StringIO::truncate() {
        wpos = rpos = 0;
    }

    bool StringIO::readline(string &line) {
        if (rpos < wpos) {
            int c;
            unsigned char *cursor;
            uint64_t pos = rpos, size;

            while (pos < wpos && (c = data[pos]) != '\n') pos++;

            cursor = data + rpos;
            size   = pos - rpos;
            rpos   = pos + 1;

            line = string((char*)cursor, size);
            return true;
        }

        return false;
    }

    char* StringIO::readline() {
        return readline(stringdata) ? (char*)stringdata.c_str() : 0;
    }
}
