#ifndef _DBICXX_PARAM_H
#define _DBICXX_PARAM_H

#include "dbic++.h"

namespace dbi {

    using namespace std;

    struct Param {
        public:
        bool isnull;
        string value;
        operator bool() { return value.length() > 0; }
    };

    Param PARAM(char* s);
    Param PARAM(string &s);
    Param PARAM(const char* s);
    Param PARAM(const dbi::null &e);
    Param PARAM_BINARY(unsigned char* data, unsigned long l);

    ostream& operator<<(ostream &out, Param &p);

}

#endif
