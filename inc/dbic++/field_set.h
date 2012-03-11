#pragma once

namespace dbi {

    /*
        Class: FieldSet
        A glorified string_list_t really, with some decorators.
        See <StringIO> for an example where you can use this.
    */
    class FieldSet {
        protected:
        string_list_t fields;

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
        std::string join(std::string delim);
    };
}
