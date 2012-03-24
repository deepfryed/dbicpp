#pragma once

namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: FileIO
        Concrete implementation of IO that can stream data from files.
    */
    class FileIO : public IO {
        protected:
        FILE *fp;
        string stringdata;
        string empty;

        public:
        /*
            Constructor: FileIO(const char*path, char* mode)
            Initializes an IO object with data from a file.

            Parameters:
            path - absolute file path.
            mode - mode to open file under (currently only "r" makes sense).
        */
        FileIO(const char *path, char *mode);
        ~FileIO();

        string&  read();
        uint32_t read(char *, uint32_t);

        void write(const char *);
        void write(const char *, uint32_t);

        void truncate();

        bool  readline(string &);
        char* readline();
    };

}

