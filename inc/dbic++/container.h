#ifndef _DBICXX_CONTAINER_H
#define _DBICXX_CONTAINER_H

#include <map>
#include <vector>
#include <sstream>

namespace dbi {

    using namespace std;

    class Param : public string {
        private:
        bool nullvalue;
        public:
        Param()              : string("") { nullvalue = true; };
        Param(char *s)       : string(s)  { nullvalue = false; };
        Param(const char *s) : string(s)  { nullvalue = false; };
        Param(string &s)     : string(s)  { nullvalue = false; };
        bool isnull() { return nullvalue; }

    };

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
