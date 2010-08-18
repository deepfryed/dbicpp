#ifndef _DBICXX_RESULTROW_H
#define _DBICXX_RESULTROW_H

namespace dbi {

    using namespace std;

    /*
        Class: ResultRow
        Encapsulates an array of parameters in a result.
    */
    class ResultRow : public vector<Param> {
        public:
        ResultRow() {}
        /*
            Constructor: ResultRow(int, ...)
            Builds a row of a given length with string data, useful if you want
            to quickly create a vector<Param> instance.

            (begin code)
            ResultRow fields(3, "id", "user", "name");
            (end)

        */
        ResultRow(int n, ...);

        /*
            Function: join(string)
            Concatenates the row data delimited by the string given.

            Parameters:
            delim - string used as a delimiter.

            Returns:
            value - concatenated string.
        */
        string join(string delim);

        /*
            Operator: <<(string)
            Shorcut for push_back() method.
        */
        void operator<<(string v);

        /*
            Operator: <<(Param&)
            Shorcut for push_back() method.
        */
        void operator<<(Param &v);

        operator bool() { return size() > 0; }
        friend ostream& operator<< (ostream &out, ResultRow &r);
    };

}

#endif
