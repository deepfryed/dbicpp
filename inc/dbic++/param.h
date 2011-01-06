#ifndef _DBICXX_PARAM_H
#define _DBICXX_PARAM_H

#include "dbic++.h"

namespace dbi {

    using namespace std;

    /*
        Struct: Param
        Encapsulates a value in string or binary format.

        (begin code)
        struct Param {
            bool   isnull;
            string value;
            bool   binary;
        };
        (end)
    */
    struct Param {
        bool   isnull;
        string value;
        bool   binary;
    };

    /*
        Function: PARAM(char*)
        Creates a Param given a string.
    */
    Param PARAM(char* s);
    /*
        Function: PARAM(string&)
        Creates a Param given a string.
    */
    Param PARAM(string &s);
    /*
        Function: PARAM(const char*)
        Creates a Param given a string.
    */
    Param PARAM(const char* s);
    /*
        Function: PARAM(null())
        Creates a special NULL Param.
    */
    Param PARAM(const dbi::null &e);
    /*
        Function: PARAM(unsigned char*, uint64_t)
        Creates a Param given a string that may not be '\0' terminated.
    */
    Param PARAM(unsigned char* data, uint64_t l);
    /*
        Function: PARAM_BINARY(unsigned char*, uint64_t)
        Creates a Param given binary data.
    */
    Param PARAM_BINARY(unsigned char* data, uint64_t l);

    ostream& operator<<(ostream &out, Param &p);

}

#endif
