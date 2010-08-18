namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    /*
        Class: IOFileStream
        Concrete implementation of IO that can stream data from files.
    */
    class IOFileStream : public IOStream {
        private:
        int fd;
        public:
        IOFileStream() {}
        ~IOFileStream();
        /*
            Constructor: IOFileStream(const char*, uint)
            Initializes an IO object with data from a file.

            Parameters:
            path - absolute file path.
            mode - mode to open file under (currently only O_RDONLY makes sense).
        */
        IOFileStream(const char *path, uint mode);
        string &read(void);
        uint read(char *buffer, uint);
    };

}

