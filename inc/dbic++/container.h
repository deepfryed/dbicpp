#ifndef _DBICXX_CONTAINER_H
#define _DBICXX_CONTAINER_H

#include <map>
#include <vector>
#include <sstream>

#include "dbic++/param.h"

namespace dbi {

    using namespace std;

    class ResultRow : public vector<Param> {
        public:
        ResultRow() {}
        string join(string delim);
        void operator<<(string v);
        void operator<<(Param &v);
        friend ostream& operator<< (ostream &out, ResultRow &r);
    };

    class ResultRowHash : public map<string,Param> {
        public:
        ResultRowHash() {}
        vector<string> columns();
        friend ostream& operator<< (ostream &out, ResultRowHash &r);
    };
}

#endif
