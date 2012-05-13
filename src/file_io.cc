#include "dbic++.h"

#ifndef HAS_GETLINE
size_t getline(char **lineptr, size_t *size, FILE *fp) {
    if (!*lineptr) *lineptr = (char*)malloc(*size + 1);
    if (!*lineptr) return -1;

    if (fgets(*lineptr, *size, fp) != NULL)
      return strlen(*lineptr);
    else
      return -1;
}
#endif

namespace dbi {
    FileIO::FileIO(const char *path, char* mode) {
        if (!(fp = fopen(path, mode)))
            throw RuntimeError(strerror(errno));
    }

    string& FileIO::read() {
        uint32_t count;
        char buffer[16384];
        count = ::read(fileno(fp), buffer, 16384);
        if (count > 0) {
            stringdata = string(buffer, count);
            return stringdata;
        }

        return empty;
    }

    uint32_t FileIO::read(char *buffer, uint32_t len) {
        len = ::read(fileno(fp), buffer, len);
        return len;
    }

    FileIO::~FileIO() {
        if (fp) fclose(fp);
    }

    void FileIO::truncate() {
        // NOP
    }

    void FileIO::write(const char *data) {
        write(data, strlen(data));
    }

    void FileIO::write(const char *data, uint32_t size) {
        if (fp)
            size = ::write(fileno(fp), data, size);
    }

    bool FileIO::readline(string &line) {
        size_t size;
        char *buffer = 0;

        size = getline(&buffer, &size, fp);
        if (size >= 0) {
            line = string(buffer, size);
            free(buffer);
            return true;
        }

        return false;
    }

    char* FileIO::readline() {
        return readline(stringdata) ? (char*)stringdata.c_str() : 0;
    }
}
