#include "dbic++.h"

namespace dbi {

    string ResultRow::join(string delim) {
        ostringstream o;
        if (size() > 0) {
            for (unsigned int i = 0; i < size(); i++) {
                o << at(i);
                if (i >= 0 && i < size() - 1) o << delim;
            }
        }
        return o.str();
    }

    void ResultRow::operator<<(string v) {
        this->push_back(v);
    }
    
    vector<string> ResultRowHash::columns(void) {
        vector<string> rs;
        for(map<string,string>::iterator iter = begin(); iter != end(); ++iter)
            rs.push_back(iter->first);
        return rs;
    }

    ostream& operator<< (ostream &out, ResultRow &r) {
        out << r.join("\t");
        return out;
    }

    ostream& operator<< (ostream &out, ResultRowHash &r) {
        for(map<string,string>::iterator iter = r.begin(); iter != r.end();) {
            out << iter->first << "\t" << iter->second;
            if (++iter != r.end()) out << "\t";
        }
        return out;
    }
}
