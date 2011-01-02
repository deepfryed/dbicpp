namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: AbstractResult
        Pure virtual class that defines the api that individual database drivers need to support.
        Use this as a reference only. It is recommended to use the Statement class for any real work.
    */
    class AbstractResult {
        public:
        void *context;

        /*
            Function: rows
            Returns the number of rows in the result.

            Returns:
            rows - rows in result.
        */
        virtual uint32_t rows() = 0;

        /*
            Function: columns
            Returns the number of columns in the result.

            Returns:
            columns - columns in result.
        */
        virtual uint32_t columns() = 0;

        /*
            Function: fields
            Returns the fields returned in the result.

            Returns:
            fields - vector<string>
        */
        virtual vector<string>& fields() = 0;

        /*
            Function: read(ResultRow&)
            Reads a row from result as ResultRow.

            Parameters:
            row - ResultRow passed by reference.

            Returns:
            true or false - false if end of results reached, true otherwise.
        */
        virtual bool read(ResultRow& row) = 0;

        /*
            Function: read(ResultRowHash &)
            Reads a row from result as ResultRowHash.

            Parameters:
            row - ResultRowHash passed by reference.

            Returns:
            true or false - false if end of results reached, true otherwise.
        */
        virtual bool read(ResultRowHash& row) = 0;

        /*
            Function: read(uint32_t, uint32_t, uint64_t*)
            Reads a field at a given position from result.

            Parameters:
            rowno - row number (0 - rows()-1)
            colno - column number (0 - columns()-1)
            len   - pointer to uint64_t that is populated with field length if provided.

            Returns:
            char* - pointer to data value or NULL if it was a NULL value.

            *Important*: The pointer returned should not be freed and the address may be
                        reused in subsequent calls.
        */
        virtual unsigned char* read(uint32_t r, uint32_t c, uint64_t* len) = 0;

        /*
            Function: tell
            Returns the current row number in the result set that a subsequent read
            will return.

            Returns:
            rowno - row number.
        */
        virtual uint32_t tell() = 0;

        /*
            Function: seek
            Seek to a row in the result set.

            Parameters:
            rowno - row number.
        */
        virtual void seek(uint32_t) = 0;

        /*
            Function: rewind
            Reset the row number pointer to first row. This is same as seek(0).
        */
        virtual void rewind() = 0;

        /*
            Function: lastInsertID
            Returns the last numeric value automatically inserted.

            Returns:
            id - uint64_t value.

            *Important*: You need to add a RETURNING clause when using PostgreSQL.

            (start code)
                Handle h ("postgresql", getlogin(), "", "test");
                h.execute("create table users(id serial, name text, email text, primary key (id))");
                Statement stmt = h.prepare("insert into users(name) values(?) returning id");
                stmt << "jonah", execute();
                cout << "inserted row with id: " << stmt.lastInsertID() << endl;
            (end)
        */
        virtual uint64_t lastInsertID() = 0;

        /*
            Function: types
            Returns an array of field types in the resultset.

            Returns:
            types - vector<int> signifying a type for each field. Refer to the following constants.

                - DBI_TYPE_INT
                - DBI_TYPE_TIME
                - DBI_TYPE_TEXT
                - DBI_TYPE_FLOAT
                - DBI_TYPE_NUMERIC
                - DBI_TYPE_BOOLEAN
                - DBI_TYPE_BLOB

        */
        virtual vector<int>& types() = 0;

        /*
            Function: cleanup
            Cleanup local buffers before deallocation. Usually you would not need to
            call this. Statement::~Statement() will do it for you.
        */
        virtual void cleanup() = 0;

        // ASYNC API
        // Returns false if done, true is still more probably yet to consume
        virtual bool consumeResult() = 0;
        // Once all available data has been consumed, prepare results for
        // access.
        virtual void prepareResult() = 0;

        virtual ~AbstractResult() {};
    };

}

