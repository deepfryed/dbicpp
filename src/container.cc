#include "dbic++.h"
#include <cstdio>

namespace dbi {

    //--------------------------------------------------------------------------
    // ResultRow
    //--------------------------------------------------------------------------

    void ResultRow::resize(int n) {
        fields.resize(n);
    }

    void ResultRow::clear() {
        fields.clear();
    }

    int ResultRow::size() {
        return fields.size();
    }

    Param& ResultRow::operator[](int n) {
        return n < fields.size() ? fields[n] : nil;
    }

    ostream& operator<< (ostream &out, ResultRow &r) {
        for (int n = 0; n < r.fields.size() - 1; n++)
            out << r[n].value << "\t";
        out << r[r.size()-1].value;
        return out;
    }

    //--------------------------------------------------------------------------
    // ResultRowHash
    //--------------------------------------------------------------------------

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

    //--------------------------------------------------------------------------
    // FieldSet
    //--------------------------------------------------------------------------

    FieldSet::FieldSet(int n, ...) {
        char *ptr;
        va_list ap;
        va_start(ap, n);
        for (int i = 0; i < n; i++) {
            ptr = va_arg(ap, char *);
            fields.push_back(ptr ? string(ptr) : "");
        }
        va_end(ap);
    }

    string FieldSet::join(string delim) {
        if (size() > 0) {
            unsigned i;
            string out;
            for (i = 0; i < fields.size() - 1; i++)
                out += fields[i] + delim;
            out += fields[fields.size() - 1];
            return out;
        }
        return "";
    }

    int FieldSet::size() {
        return fields.size();
    }
}
