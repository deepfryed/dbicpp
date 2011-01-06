#include "dbic++.h"

namespace dbi {
    Param PARAM(char *s) {
        Param p = { false, s ? s : "", false };
        return p;
    }

    Param PARAM(const char *s) {
        Param p = { false, s ? s : "", false };
        return p;
    }

    Param PARAM(string &s) {
        Param p = { false, s, false };
        return p;
    }

    Param PARAM(unsigned char *data, uint64_t l) {
        Param p = { false, data ? string((const char*)data, l) : "", false };
        return p;
    }

    Param PARAM_BINARY(unsigned char *data, uint64_t l) {
        Param p = { false, data ? string((const char*)data, l) : "", true };
        return p;
    }

    Param PARAM(const dbi::null &e) {
        Param p = { true, "", false };
        return p;
    }

    ostream& operator<<(ostream &out, Param &p)  {
        out << (p.isnull ? "\\N" : p.value);
        return out;
    }
}
