#pragma once

namespace dbi {

    class Result;

    /*
        Class: Statement
        Facade for AbstractStatement, you should be using this for most of the normal work. The class
        wraps the underlying database adapter and deallocates memory allocated from heap when destroyed.

        See <Handle> for an example.
    */
    class Statement {
        protected:
        AbstractHandle *h;
        AbstractStatement *st;
        param_list_t params;

        public:
        Statement();
        /*
            Constructor: Statement(AbstractStatement*)
            Constructs a Statement instance given an abstract statement.

            Parameters:
            stmt - AbstractStatement instance.
        */
        Statement(AbstractStatement *stmt);

        /*
            Constructor: Statement(Handle&)
            Constructs a Statement instance given a handle.

            Parameters:
            handle - Handle instance.
        */
        Statement(Handle &handle);

        /*
            Constructor: Statement(Handle&, string)
            Constructs a Statement instance given a handle and a sql.

            Parameters:
            handle - Handle instance.
            sql    - SQL to prepare using the handle.
        */
        Statement(Handle &handle, std::string sql);

        /*
            Constructor: Statement(Handle *)
            Constructs a Statement instance given a pointer to handle.

            Parameters:
            handle - Pointer to Handle instance.
        */
        Statement(Handle *handle);

        /*
            Constructor: Statement(Handle *, string)
            Constructs a Statement instance given a pointer to a handle and a sql.

            Parameters:
            handle - Pointer to a Handle instance.
            sql    - SQL to prepare using the handle.
        */
        Statement(Handle *handle, std::string sql);

        ~Statement();

        /*
            Function: bind(long)
            Bind a long value to statement.

            Parameters:
            value - long
        */
        void bind(long value);

        /*
            Function: bind(double)
            Bind a double value to statement.

            Parameters:
            value - double
        */
        void bind(double value);

        /*
            Function: bind(Param)
            Bind a string or binary value to statement.

            Parameters:
            value - Param
        */
        void bind(Param value);

        /*
            Operator: << (string)
            Dealloactes any work memory and prepares a new statement for execution.
        */
        Statement& operator<<(std::string sql);

        /*
            Operator: , (string)
            Alias for bind(Param)

            Parameters:
            value - string
        */
        Statement& operator,(std::string value);
        /*
            Operator: % (string)
            Alias for bind(Param)

            Parameters:
            value - string
        */
        Statement& operator%(std::string value);

        /*
            Operator: , (long)
            Alias for bind(long)

            Parameters:
            value - long
        */
        Statement& operator,(long value);

        /*
            Operator: % (long)
            Alias for bind(long)

            Parameters:
            value - long
        */
        Statement& operator%(long value);

        /*
            Operator: , (double)
            Alias for bind(double)

            Parameters:
            value - double
        */
        Statement& operator,(double value);

        /*
            Operator: % (double)
            Alias for bind(double)

            Parameters:
            value - double
        */
        Statement& operator%(double value);

        /*
            Operator: , (null())
            Alias for bind(PARAM(null()))

            Parameters:
            value - null()
        */
        Statement& operator,(dbi::null const &e);

        /*
            Operator: % (null())
            Alias for bind(PARAM(null()))

            Parameters:
            value - null()
        */
        Statement& operator%(dbi::null const &e);

        /*
            Operator: , (execute())
            Alias for execute()

            This is mostly syntactic sugar allowing you to do,
            (start code)
                Statement stmt (handle, "update users set verified = true where id = ? and name like ?")
                stmt % 1L, "jon%", execute();
            (end)
        */
        uint32_t operator,(dbi::execute const &);

        /*
            Function: execute
            Executes the prepared statement along with the bind parameters
            bound using bind functions or operators.

            Returns:
            rows - The number of rows affected or returned.
        */
        uint32_t execute();

        /*
            Function: execute(param_list_t&)
            See <AbstractStatement::execute(param_list_t&)>
        */
        uint32_t execute(param_list_t &bind);

        /*
            Function: result
            Returns a pointer to a result object. This needs to be
            deallocated explicitly.

            Returns:
            Result* - Pointer to the Result set object.
        */
        Result* result();

        /*
            Function: cleanup
            Releases local buffers and deallocates any temporary memory.
        */
        void cleanup();
        void finish();

        /*
            Function: lastInsertID
            See <AbstractResult::lastInsertID()>
        */
        uint64_t lastInsertID();

    };
}
