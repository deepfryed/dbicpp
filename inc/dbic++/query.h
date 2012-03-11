namespace dbi {

    class Result;
    class Statement;

    /*
        Class: Query
        Wraps the Statement & Result classes.

        See <Handle> for an example.
    */
    class Query : public Result, public Statement {
        public:
        /*
            Constructor: Query(Handle&, string)

            Parameters:
            handle - Handle instance.
            sql    - SQL to prepare using the handle.
        */
        Query(Handle &handle, std::string sql);
        ~Query();

        void finish();
        void cleanup();

        uint32_t execute();
        uint32_t execute(param_list_t &bind);

        /*
            Operator: , (string)
            Alias for bind(Param)

            Parameters:
            value - string
        */
        Query& operator,(std::string value);
        /*
            Operator: % (string)
            Alias for bind(Param)

            Parameters:
            value - string
        */
        Query& operator%(std::string value);

        /*
            Operator: , (long)
            Alias for bind(long)

            Parameters:
            value - long
        */
        Query& operator,(long value);

        /*
            Operator: % (long)
            Alias for bind(long)

            Parameters:
            value - long
        */
        Query& operator%(long value);

        /*
            Operator: , (double)
            Alias for bind(double)

            Parameters:
            value - double
        */
        Query& operator,(double value);

        /*
            Operator: % (double)
            Alias for bind(double)

            Parameters:
            value - double
        */
        Query& operator%(double value);

        /*
            Operator: , (null())
            Alias for bind(PARAM(null()))

            Parameters:
            value - null()
        */
        Query& operator,(dbi::null const &e);

        /*
            Operator: % (null())
            Alias for bind(PARAM(null()))

            Parameters:
            value - null()
        */
        Query& operator%(dbi::null const &e);

        /*
            Operator: , (execute())
            Alias for execute()

            This is mostly syntactic sugar allowing you to do,
            (start code)
                Query query (handle, "select * from users where id = ? and name like ?")
                query % 1L, "jon%", execute();
            (end)
        */
        uint32_t operator,(dbi::execute const &);

        Query& operator<<(std::string sql);
    };
}
