namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: AbstractHandle
        Pure virtual class that defines the api that individual database drivers need to support.
        Use this as a reference only. It is recommended to use the Handle class for any real work.
    */
    class AbstractHandle {
        public:
        /*
            Function: prepare(string)
            prepares a SQL and returns a statement handle.

            Parameters:
            sql - SQL statement.

            Returns:
            A pointer to AbstractStatement.
        */
        virtual AbstractStatement* prepare(string sql) = 0;

        /*
            Function: execute(string)
            Executes a SQL.

            Parameters:
            sql - SQL without placeholders.

            Returns:
            rows - number of rows affected or returned.
        */
        virtual uint32_t execute(string sql) = 0;

        /*
            Function: execute(string, vector<Param>&)
            executes a SQL with bind values.

            Parameters:
            sql  - SQL with or without placeholders.
            bind - vector<Param> that contains bind values. Refer to the <Param> struct and associated methods.

            Returns:
            rows - number of rows affected or returned.
        */
        virtual uint32_t execute(string sql, vector<Param> &bind) = 0;

        /*
            Function: begin
            Starts a transaction. This will rollback any previous uncommited transactions on this handle.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool begin() = 0;

        /*
            Function: commit
            Commits a transaction.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool commit() = 0;

        /*
            Function: rollback
            Rollback a transaction.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool rollback() = 0;

        /*
            Function: begin(string)
            Begin a savepoint.

            Parameters:
            name - Savepoint name. This is optional, if none is provided usually a UUID is used.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool begin(string name) = 0;

        /*
            Function: commit(string)
            Commits a savepoint.

            Parameters:
            name - Savepoint name. If none provided it will commit the most recent savepoint.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool commit(string name) = 0;

        /*
            Function: rollback(string)
            Rollback a savepoint.

            Parameters:
            name - Savepoint name. If none provided it will rollback the most recent savepoint.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool rollback(string name) = 0;

        virtual void* call(string name, void*, uint64_t) = 0;

        /*
            Function: close
            Close connection.

            Returns:
            true or false - denotes success or failure.
        */
        virtual bool close() = 0;

        /*
            Function: cleanup
            Cleanup connection and release any state buffers. This will be usually called by
            the destructor in Handle class.
        */
        virtual void cleanup() = 0;

        /*
            Function: write(string, FieldSet&, IO*)
            Bulk write data into a database table.

            Parameters:
            table  - table name.
            fields - field names.
            io     - pointer to an IO object.

            *Tip*: Use the FieldSet constructor to create a field name array.

            (begin code)
            FieldSet fields(3, "id", "name", "age");
            (end)

            Returns:
            rows   - The number of rows written to database.
        */
        virtual uint64_t write(string table, FieldSet &fields, IO*) = 0;

        /*
            Function: results
            Returns the Result associated with a previous query.

            Returns:
            results - An object conforming to AbstractResult.
                      NULL If the previous query did not return any results.

            *Important*: You need to call cleanup() on the result set before deleting it and
                        then deleted it when done with it.
        */
        virtual AbstractResult* results() = 0;

        /*
            Function: setTimeZoneOffset(int, int)
            Sets the connection timezone offset.

            Parameters:
            hours  - Offset hours from UTC.
            mins   - Offset mins from UTC.
        */
        virtual void setTimeZoneOffset(int hours, int mins) = 0;

        /*
            Function: setTimeZone(char *)
            Sets the connection timezone as a string conforming to zoneinfo.

            Parameters:
            zone - Timezone name in zoneinfo format.
        */
        virtual void setTimeZone(char *zone) = 0;

        /*
            Function: escape(string)
            Escapes a string value for use in SQL queries.

            Parameters:
            value  - string value to be escaped.

            Returns:
            value  - string value that is escaped for the underlying database.
        */
        virtual string escape(string) = 0;

        /*
            Function: driver
            Returns the driver for this handle.

            Returns:
            name - string value.
        */
        virtual string driver() = 0;

        friend class ConnectionPool;
        friend class Request;

        // ASYNC API
        protected:
        virtual int socket() = 0;
        virtual AbstractResult* aexecute(string sql, vector<Param> &bind) = 0;
        virtual void initAsync() = 0;
        virtual bool isBusy() = 0;
        virtual bool cancel() = 0;
    };

}

