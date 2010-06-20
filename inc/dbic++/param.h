#ifndef _DBICXX_PARAM_H
#define _DBICXX_PARAM_H

namespace dbi {
    struct Param {
        bool isnull;
        string value;
    };

    Param PARAM(char* s);
    Param PARAM(string &s);
    Param PARAM(const char* s);
    Param PARAM(const dbi::null &e);
    Param PARAM_BINARY(unsigned char* data, int l);

    ostream& operator<<(ostream &out, Param &p);
}

#endif
