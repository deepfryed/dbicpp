#include "dbic++.h"

namespace dbi {
    Param PARAM(char *s) {
        Param p = { false, s, false, strlen(s) };
        return p;
    }

    Param PARAM(const char *s) {
        Param p = { false, s, false, strlen(s) };
        return p;
    }

    Param PARAM(string &s) {
        Param p = { false, s, false, s.length() };
        return p;
    }

    Param PARAM(unsigned char *data, uint64_t l) {
        Param p = { false, string((const char*)data, l), false, l };
        return p;
    }

    Param PARAM_BINARY(unsigned char *data, uint64_t l) {
        Param p = { false, string((const char*)data, l), true, l };
        return p;
    }

    Param PARAM(const dbi::null &e) {
        Param p = { true, "", false, 0 };
        return p;
    }

    ostream& operator<<(ostream &out, Param &p)  {
        out << (p.isnull ? "\\N" : p.value);
        return out;
    }
}
