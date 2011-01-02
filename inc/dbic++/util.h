namespace dbi {

    /*
        Section: Utility functions
    */
    class Driver {
        public:
        Driver() {};
        ~Driver() { dlclose(handle); }
        string name;
        string version;
        void *handle;
        AbstractHandle* (*connect)(string user, string pass, string dbname, string host, string port);
    };

    bool dbiInitialize(string path = DEFAULT_DRIVER_PATH);
    void dbiShutdown();

    bool trace();

    /*
        Function: trace(bool)
        Turns on or off printing sql statements that are executed.

        Parameters:
        flag - bool
    */
    void trace(bool flag);

    /*
        Function: trace(bool, int)
        Turns on or off printing sql statements that are executed to a specific file handle.

        Parameters:
        flag - bool
        fd   - file descriptor
    */
    void trace(bool flag, int fd);

    void logMessage(int fd, string msg);
    string formatParams(string sql, vector<Param> &p);

    string generateCompactUUID();
}
