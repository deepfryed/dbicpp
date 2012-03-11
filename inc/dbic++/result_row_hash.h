#ifndef _DBICXX_RESULTROW_HASH_H
#define _DBICXX_RESULTROW_HASH_H

namespace dbi {

    /*
        Class: ResultRowHash
        Encapsulates a hash that maps field names to values in a result.
    */
    class ResultRowHash {
        private:
        map<string,Param> data;
        public:
        ResultRowHash() {}
        /*
            Function: fields
            Returns the fields in the result.

            Returns:
            fields - string_list_t
        */
        string_list_t fields();
        operator bool() { return data.size() > 0; }
        /*
            Operator: [](const char*)
            Returns the value given a field name.

            Returns:
            value - Param
        */
        Param& operator [](const char*);
        /*
            Operator: [](string &)
            Returns the value given a field name.

            Returns:
            value - Param
        */
        Param& operator [](std::string&);
        void clear();
        friend std::ostream& operator<< (std::ostream &out, ResultRowHash &r);
    };
}

#endif
