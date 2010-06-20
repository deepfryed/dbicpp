#include "dbic++.h"

namespace dbi {

    Param::Param()                     { init(false); }
    Param::Param(const dbi::null &e)   { init(true);  }
    Param::Param(string &s)            { init(false); write(s); }
    Param::Param(const char *s)        { init(false); write(s); }
    Param::Param(char *s)              { init(false); write(s); }
    Param::Param(const Param &p)       { init(p._isnull); write(p._buffer.str().c_str()); }

    void Param::write(string &s)               { _buffer.write(s.c_str(), s.length()); }
    void Param::write(char *s)                 { _buffer.write((const char*)s, strlen(s)); }
    void Param::write(const char *s)           { _buffer.write(s, strlen(s)); }
    void Param::write(unsigned char *s, int n) { _buffer.write((char *) s, n); }

    string& Param::str()                       { _str = _buffer.str(); return _str; }
    bool Param::isnull()                       { return _isnull; }
    void Param::clear()                        { _buffer.str(""); }

    Param& Param::operator +=(Param p)         { write(p.str().c_str()); return *this; }
    Param& Param::operator =(Param p)          { init(p._isnull); write(p.str().c_str()); return *this; }
    Param  Param::operator +(Param p)          { Param n = *this; return n += p; }

    void Param::init(bool v) {
        _isnull = v; clear();
    }

    ostream& operator<<(ostream &out, Param &p)  {
        out << (p._isnull ? "\\N" : p._buffer.str());
        return out;
    }
}
