#ifndef _DBICXX_CONTAINER_H
#define _DBICXX_CONTAINER_H

#include <map>
#include <vector>
#include <sstream>

namespace dbi {

    using namespace std;

    class ResultRow : public vector<string> {
        public:
        ResultRow() {}
        string join(string delim);
        void operator<<(string v);
        friend ostream& operator<< (ostream &out, ResultRow &r);
    };

    class ResultRowHash : public map<string,string> {
        public:
        ResultRowHash() {}
        vector<string> columns();
        friend ostream& operator<< (ostream &out, ResultRowHash &r);
    };
}

#endif
