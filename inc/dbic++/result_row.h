#pragma once
#ifndef _DBICXX_RESULTROW_H
#define _DBICXX_RESULTROW_H

#include <string>
#include <vector>

namespace dbi {

    /*
        Class: ResultRow
        Encapsulates an array of parameters in a result.
    */
    class ResultRow {
        protected:
        param_list_t fields;
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
        friend std::ostream& operator<< (std::ostream &out, ResultRow &r);
    };

}

#endif
