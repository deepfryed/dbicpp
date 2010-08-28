namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: IOStream
        Concrete implementation of IO that can handle string data. Generally useful when
        you just want to insert data into a table (or fetch data from a table) without
        the overhead of multiple statement execution.


        Example:
        (begin code)
        #include "dbic++.h"
        #include <unistd.h>

        using namespace std;
        using namespace dbi;

        int main() {
            ResultRowHash res;
            Handle h ("mysql", getlogin(), "", "dbicpp");

            // Set trace on and log queries to stderr
            trace(true, fileno(stderr));

            IOStream buffer;

            // columns are tab separated
            // rows are terminated by a single newline
            buffer.write("sally\tsally@local\n");
            buffer.write("jonas\tjonas@local\n");

            FieldSet fields(2, "name", "email");
            cout << "Wrote "
                 << h.write("users", fields, &buffer)
                 << " rows" << endl;

            Statement stmt(h, "select * from users");
            stmt.execute();
            while (stmt.read(res))
                cout << res["id"]    << "\t"
                     << res["name"]  << "\t"
                     << res["email"] << endl;
            }
        }
        (end)
    */
    class IOStream : public IO {
        private:
        bool eof;
        uint32_t loc;
        protected:
        string empty;
        string data;
        public:
        IOStream() { eof = false; loc = 0; }

        /*
            Constructor: IOStream(const char*, uint64_t)
            See <IO::IO(const char*, uint64_t)>
        */
        IOStream(const char *, uint64_t);

        /*
            Function: write(const char*)
            See <IO::write(const char*)>
        */
        void write(const char *);

        /*
            Function: write(const char*, uint64_t)
            See <IO::write(const char*, uint64_t)>
        */
        void write(const char *, uint64_t);

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
        uint32_t read(char *buffer, uint32_t);
    };
}

