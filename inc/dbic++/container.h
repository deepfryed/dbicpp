#ifndef _DBICXX_CONTAINER_H
#define _DBICXX_CONTAINER_H

#include <sstream>

namespace dbi {

    using namespace std;

    class ResultRow : public vector<Param> {
        public:
        ResultRow() {}
        string join(string delim);
        void operator<<(string v);
        void operator<<(Param &v);
        operator bool() { return size() > 0; }
        friend ostream& operator<< (ostream &out, ResultRow &r);
    };

    class ResultRowHash {
        private:
        map<string,Param> data;
        public:
        ResultRowHash() {}
        vector<string> columns();
        operator bool() { return data.size() > 0; }
        Param& operator [](const char*);
        Param& operator [](string&);
        void clear();
        friend ostream& operator<< (ostream &out, ResultRowHash &r);
    };
}

#endif
