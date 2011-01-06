#ifndef _DBICXX_RESULTROW_H
#define _DBICXX_RESULTROW_H

namespace dbi {

    using namespace std;

    /*
        Class: ResultRow
        Encapsulates an array of parameters in a result.
    */
    class ResultRow {
        protected:
        vector<Param> fields;
        Param nil;

        public:
        ResultRow() {}

        /*
            Function: resize(int columns)
            Sets up the number of fields in the row.

            Parameters:
            columns - number of columns in a row.
        */
        void resize(int);

        /*
            Function: clear()
            Clears a row completely, you need to call resize after this if you intend to store column data.
            Same as calling resize(0).
        */
        void clear();

        /*
            Function: size()
            Returns the number of columns in a row.

            Returns:
            integer
        */
        int  size();

        /*
            Operator: [](int n)
            Returns the column data at given position.

            Parameters:
            n - position

            Returns:
            Reference to <Param>
        */
        Param& operator[](int);
        friend ostream& operator<< (ostream &out, ResultRow &r);
    };

    /*
        Class: FieldSet
        A glorified vector<string> really, with some decorators.
        See <StringIO> for an example where you can use this.
    */
    class FieldSet {
        protected:
        vector<string> fields;

        public:
        /*
            Constructor: FieldSet(int, ...)
            (begin code)
            FieldSet fields(3, "id", "user", "name");
            (end)

        */
        FieldSet(int n, ...);

        /*
            Function: size()
            Returns the number of fields.

            Returns:
            integer
        */
        int size();

        /*
            Function: join(string delim)
            Concatenates the field names with a given delimiter.

            Parameters:
            delim - a string delimiter.

            Returns:
            string
        */
        string join(string delim);
    };

}

#endif
