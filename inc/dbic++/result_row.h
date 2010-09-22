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

    /*
        Class: FieldSet
        A glorified vector<string> really, with some decorators.
        See <IOStream> for an example where you can use this.
    */
    class FieldSet : public ResultRow {
        public:
        FieldSet() {}
        /*
            Constructor: FieldSet(int, ...)
            (begin code)
            FieldSet fields(3, "id", "user", "name");
            (end)

        */
        FieldSet(int n, ...);

        FieldSet(vector<string>);
    };

}

#endif
