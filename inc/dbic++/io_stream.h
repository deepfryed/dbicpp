namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: IOStream
        Concrete implementation of IO that can handle string data.
    */
    class IOStream : public IO {
        private:
        bool eof;
        uint loc;
        protected:
        string empty;
        string data;
        public:
        IOStream() { eof = false; loc = 0; }

        /*
            Constructor: IOStream(const char*, ulong)
            See <IO::IO(const char*, ulong)>
        */
        IOStream(const char *, ulong);

        /*
            Function: write(const char*)
            See <IO::write(const char*)>
        */
        void write(const char *);

        /*
            Function: write(const char*, ulong)
            See <IO::write(const char*, ulong)>
        */
        void write(const char *, ulong);

        /*
            Function: writef(const char *fmt, ...)
            Appends formatted data to io object. Similar to sprintf.

            Parameters:
            fmt - format similar to what printf family of functions expect.
        */
        void writef(const char *fmt, ...);

        /*
            Function: truncate
            See <IO::truncate>
        */
        void truncate(void);

        string &read(void);
        uint read(char *buffer, uint);
    };
}

