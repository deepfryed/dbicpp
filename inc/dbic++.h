#ifndef _DBICXX_BASE_H
#define _DBICXX_BASE_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <map>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pcrecpp.h>
#include <uuid/uuid.h>

#undef uint
#undef ulong
#define unint unsigned int
#define ulong unsigned long

namespace dbi {
    struct null {};
    struct execute {};
}

#include "dbic++/error.h"
#include "dbic++/param.h"
#include "dbic++/container.h"
#include "dbic++/cpool.h"

#define DEFAULT_DRIVER_PATH "/usr/lib/dbic++"

#define DBI_TYPE_INT     1
#define DBI_TYPE_TIME    2
#define DBI_TYPE_TEXT    3
#define DBI_TYPE_FLOAT   4
#define DBI_TYPE_NUMERIC 5
#define DBI_TYPE_BOOLEAN 6
#define DBI_TYPE_BLOB    7

namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    extern bool _trace;
    extern int  _trace_fd;

    class AbstractStatement;
    class AbstractResultSet;

    class IO {
        public:
        IO() {}
        IO(const char *, ulong) {}
        virtual string &read(void) = 0;
        virtual uint read(char *buffer, uint) = 0;
        virtual void write(const char *) = 0;
        virtual void write(const char *, ulong) = 0;
        virtual void truncate(void) = 0;
    };

    class IOStream : public IO {
        private:
        bool eof;
        uint loc;
        protected:
        string empty;
        string data;
        public:
        IOStream() { eof = false; loc = 0; }
        IOStream(const char *, ulong);
        string &read(void);
        uint read(char *buffer, uint);
        void write(const char *);
        void write(const char *, ulong);
        void writef(const char *, ...);
        void truncate(void);
    };

    class IOFileStream : public IOStream {
        private:
        int fd;
        public:
        IOFileStream() {}
        ~IOFileStream();
        IOFileStream(const char *path, uint mode);
        string &read(void);
        uint read(char *buffer, uint);
    };

    class AbstractHandle {
        public:
        virtual uint execute(string sql) = 0;
        virtual uint execute(string sql, vector<Param> &bind) = 0;
        virtual AbstractStatement* prepare(string sql) = 0;
        virtual bool begin() = 0;
        virtual bool commit() = 0;
        virtual bool rollback() = 0;
        virtual bool begin(string name) = 0;
        virtual bool commit(string name) = 0;
        virtual bool rollback(string name) = 0;
        virtual void* call(string name, void*, ulong) = 0;
        virtual bool close() = 0;
        virtual void cleanup() = 0;
        virtual ulong write(string table, ResultRow &fields, IO*) = 0;
        // IMPORTANT: You need to call cleanup() on the result set before deleting it.
        virtual AbstractResultSet* results() = 0;
        virtual void setTimeZoneOffset(int, int) = 0;
        virtual void setTimeZone(char *) = 0;
        virtual string escape(string) = 0;

        friend class ConnectionPool;
        friend class Request;

        // ASYNC API
        protected:
        virtual int socket() = 0;
        virtual AbstractResultSet* aexecute(string sql, vector<Param> &bind) = 0;
        virtual void initAsync() = 0;
        virtual bool isBusy() = 0;
        virtual bool cancel() = 0;
    };

    class AbstractResultSet {
        public:
        void *context;
        virtual uint rows() = 0;
        virtual uint columns() = 0;
        virtual vector<string> fields() = 0;
        virtual bool read(ResultRow&) = 0;
        virtual bool read(ResultRowHash&) = 0;
        virtual unsigned char* read(uint, uint, ulong*) = 0;
        virtual bool finish() = 0;
        virtual uint tell() = 0;
        virtual void seek(uint) = 0;
        virtual void cleanup() = 0;
        virtual ulong lastInsertID() = 0;
        virtual void rewind() = 0;
        virtual vector<int>& types() = 0;

        // ASYNC API
        // Returns false if done, true is still more probably yet to consume
        virtual bool consumeResult() = 0;
        // Once all available data has been consumed, prepare results for
        // access.
        virtual void prepareResult() = 0;
    };

    class AbstractStatement : public AbstractResultSet {
        public:
        virtual string command() = 0;
        virtual uint execute() = 0;
        virtual uint execute(vector<Param> &bind) = 0;
    };

    class Driver {
        public:
        Driver() {};
        ~Driver() { dlclose(handle); }
        string name;
        string version;
        void *handle;
        AbstractHandle* (*connect)(string user, string pass, string dbname, string host, string port);
    };

    class Statement;
    class Handle {
        protected:
        vector<string> trx;
        AbstractHandle *h;
        public:
        AbstractHandle* conn();
        Handle(string driver, string user, string pass, string dbname, string host, string port);
        Handle(string driver, string user, string pass, string dbname);
        Handle(AbstractHandle *ah);
        ~Handle();
        uint execute(string sql);
        Statement prepare(string sql);
        Statement operator<<(string sql);
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);
        void* call(string name, void*, ulong);
        bool close();
        vector<string>& transactions();
        ulong write(string table, ResultRow &fields, IO*);
        // IMPORTANT: You need to call cleanup() on the result set before deleting it.
        AbstractResultSet* results();
        void setTimeZoneOffset(int, int);
        void setTimeZone(char *);
        string escape(string);

        friend class Statement;
    };

    class Statement {
        private:
        AbstractStatement *st;
        AbstractHandle *h;
        ResultRow params;
        public:
        Statement();
        Statement(AbstractStatement *);
        Statement(Handle &h);
        Statement(Handle &h, string sql);
        Statement(Handle *h);
        Statement(Handle *h, string sql);
        ~Statement();
        uint rows();
        void bind(long v);
        void bind(double v);
        void bind(Param v);
        uint execute();
        uint execute(ResultRow &bind);
        Statement& operator<<(string sql);
        Statement& operator,(string v);
        Statement& operator%(string v);
        Statement& operator,(long v);
        Statement& operator%(long v);
        Statement& operator,(double v);
        Statement& operator%(double v);
        Statement& operator,(dbi::null const &e);
        Statement& operator%(dbi::null const &e);
        uint  operator,(dbi::execute const &);
        bool read(ResultRow&);
        bool read(ResultRowHash&);
        uint columns();
        vector<string> fields();
        unsigned char* read(uint r, uint c, ulong*);
        unsigned char* operator()(uint r, uint c);
        uint tell();
        void seek(uint);
        void rewind();
        bool finish();
        void cleanup();
        ulong lastInsertID();
        vector<int>& types();
    };

    bool dbiInitialize(string path);
    void dbiShutdown();

    bool trace();
    void trace(bool flag);
    void trace(bool flag, int fd);
    void logMessage(int fd, string msg);
    string formatParams(string sql, ResultRow &p);

    string generateCompactUUID();
}

#endif
