#ifndef _DBICXX_PARAM_H
#define _DBICXX_PARAM_H

namespace dbi {
    class Param {
        protected:
        stringstream _buffer;
        string _str;
        bool _isnull;

        void init(bool);
        public:
        Param();
        Param(const dbi::null &e);
        Param(string &s);
        Param(const char *s);
        Param(char *s);
        Param(const Param &p);

        void write(string &s);
        void write(char *s);
        void write(const char *s);
        void write(unsigned char *s, int n);

        string& str();
        bool isnull();
        void clear();

        Param& operator +=(Param p);
        Param& operator =(Param p);
        Param  operator +(Param p);

        friend ostream& operator<<(ostream &out, Param &p);
    };
}

#endif
