#ifndef _DBICXX_PARAM_H
#define _DBICXX_PARAM_H

#include "dbic++.h"

namespace dbi {

    using namespace std;

    struct Param {
        bool isnull;
        string value;
    };

    Param PARAM(char* s);
    Param PARAM(string &s);
    Param PARAM(const char* s);
    Param PARAM(const dbi::null &e);
    Param PARAM_BINARY(unsigned char* data, unsigned long l);

    ostream& operator<<(ostream &out, Param &p);
}

#endif
