#include "dbic++.h"

namespace dbi {
    Param PARAM(char *s) {
        Param p = { false, s };
        return p;
    }

    Param PARAM(const char *s) {
        Param p = { false, s };
        return p;
    }

    Param PARAM(string &s) {
        Param p = { false, s };
        return p;
    }

    Param PARAM_BINARY(unsigned char *data, unsigned long l) {
        Param p = { false, string((const char*)data, l) };
        return p;
    }

    Param PARAM(const dbi::null &e) {
        Param p = { true, "" };
        return p;
    }

    ostream& operator<<(ostream &out, Param &p)  {
        out << (p.isnull ? "\\N" : p.value);
        return out;
    }
}
