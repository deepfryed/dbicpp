namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: Result
        Facade for AbstractResult, you should be using this for most of the normal work.

        See <Handle> for an example.
    */
    class Result {
        private:
        AbstractResult *rs;

        public:
        Result();
        /*
            Constructor: Result(AbstractResult*)
            Constructs a Result instance.

            Parameters:
            AbstractResult instance.
        */
        Result(AbstractResult *rs);

        ~Result();

        /*
            Function: rows
            See <AbstractResult::rows()>
        */
        uint32_t rows();

        /*
            Function: columns
            See <AbstractResult::columns()>
        */
        uint32_t columns();

        /*
            Function: fields
            See <AbstractResult::fields()>
        */
        vector<string> fields();

        /*
            Function: types
            See <AbstractResult::types()>
        */
        vector<int>& types();

        /*
            Function: read(ResultRow&)
            See <AbstractResult::read(ResultRow&)>
        */
        bool read(ResultRow&);

        /*
            Function: read(ResultRowHash&)
            See <AbstractResult::read(ResultRowHash&)>
        */
        bool read(ResultRowHash&);

        /*
            Function: read(uint32_t, uint32_t, uint64_t*)
            See <AbstractResult::read(uint32_t, uint32_t, uint64_t*)>
        */
        unsigned char* read(uint32_t r, uint32_t c, uint64_t*);

        /*
            Operator: (uint32_t, uint32_t)
            Alias for read(uint32_t, uint32_t)
        */
        unsigned char* operator()(uint32_t r, uint32_t c);

        /*
            Function: tell
            See <AbstractResult::tell()>
        */
        uint32_t tell();

        /*
            Function: seek
            See <AbstractResult::seek()>
        */
        void seek(uint32_t);

        /*
            Function: rewind
            See <AbstractResult::rewind()>
        */
        void rewind();

        /*
            Function: finish
            See <AbstractResult::finish()>
        */
        bool finish();

        /*
            Function: cleanup
            Releases local buffers and deallocates any temporary memory.
        */
        void cleanup();

        /*
            Function: lastInsertID
            See <AbstractResult::lastInsertID()>
        */
        uint64_t lastInsertID();
    };
}
