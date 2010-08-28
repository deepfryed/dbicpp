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
            Constructor: IOFileStream(const char*, uint32_t)
            Initializes an IO object with data from a file.

            Parameters:
            path - absolute file path.
            mode - mode to open file under (currently only O_RDONLY makes sense).
        */
        IOFileStream(const char *path, uint32_t mode);
        string &read(void);
        uint32_t read(char *buffer, uint32_t);
    };

}

