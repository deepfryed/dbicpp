#include "dbic++.h"
#include <cstdio>

namespace dbi {

    ResultRow::ResultRow(int n, ...) {
        char *ptr;
        va_list ap;
        va_start(ap, n);
        for (int i = 0; i < n; i++) {
            ptr = va_arg(ap, char *);
            this->push_back(PARAM(ptr));
        }
        va_end(ap);
    }

    string ResultRow::join(string delim) {
        unsigned i;
        stringstream out;

        if (size() > 0) {
            for (i = 0; i < size() - 1; i++)
                out << at(i) << delim;
            out << at(size()-1);
        }

        return out.str();
    }

    void ResultRow::operator<<(string v) {
        this->push_back(PARAM(v));
    }

    void ResultRow::operator<<(Param &v) {
        this->push_back(v);
    }

    ostream& operator<< (ostream &out, ResultRow &r) {
        out << r.join("\t");
        return out;
    }

    void ResultRowHash::clear() {
        data.clear();
    }

    Param& ResultRowHash::operator[](const char *k) {
        return data[k];
    }

    Param& ResultRowHash::operator[](string &k) {
        return data[k];
    }

    vector<string> ResultRowHash::fields(void) {
        vector<string> rs;

        for(map<string,Param>::iterator iter = data.begin(); iter != data.end(); ++iter)
            rs.push_back(iter->first);

        return rs;
    }

    ostream& operator<< (ostream &out, ResultRowHash &r) {
        for(map<string,Param>::iterator iter = r.data.begin(); iter != r.data.end();) {
            out << iter->first << "\t" << iter->second;
            if (++iter != r.data.end()) out << "\t";
        }

        return out;
    }


    FieldSet::FieldSet(int n, ...) {
        char *ptr;
        va_list ap;
        va_start(ap, n);
        for (int i = 0; i < n; i++) {
            ptr = va_arg(ap, char *);
            this->push_back(PARAM(ptr));
        }
        va_end(ap);
    }

    FieldSet::FieldSet(vector<string> names) {
        for (int i = 0; i < names.size(); i++)
            this->push_back(PARAM(names[i]));
    }
}
