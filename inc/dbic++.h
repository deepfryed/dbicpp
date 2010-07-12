#ifndef _DBICXX_BASE_H
#define _DBICXX_BASE_H

#include <iostream>
#include <string>
#include <cstdarg>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pcrecpp.h>
#include <sys/time.h>
#include <uuid/uuid.h>

namespace dbi {
    struct null {};
    struct execute {};
}

#include "dbic++/error.h"
#include "dbic++/param.h"
#include "dbic++/container.h"
#include "dbic++/cpool.h"

#define DEFAULT_DRIVER_PATH "/usr/lib/dbic++"
#define OPTIONAL_ARG(a, v) a ? a : v

namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    static bool _trace;
    static int  _trace_fd;

    class AbstractStatement;
    class AbstractResultSet;

    class AbstractHandle {
        public:
        virtual unsigned int execute(string sql) = 0;
        virtual unsigned int execute(string sql, vector<Param> &bind) = 0;
        virtual AbstractStatement* prepare(string sql) = 0;
        virtual bool begin() = 0;
        virtual bool commit() = 0;
        virtual bool rollback() = 0;
        virtual bool begin(string name) = 0;
        virtual bool commit(string name) = 0;
        virtual bool rollback(string name) = 0;
        virtual void* call(string name, void*) = 0;
        virtual bool close() = 0;
        virtual void cleanup() = 0;

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
        virtual unsigned int rows() = 0;
        virtual unsigned int columns() = 0;
        virtual vector<string> fields() = 0;
        virtual ResultRow& fetchRow() = 0;
        virtual ResultRowHash& fetchRowHash() = 0;
        virtual bool finish() = 0;
        virtual unsigned char* fetchValue(unsigned int, unsigned int, unsigned long*) = 0;
        virtual unsigned int currentRow() = 0;
        virtual void advanceRow() = 0;
        virtual void cleanup() = 0;
        virtual unsigned long lastInsertID() = 0;

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
        virtual unsigned int execute() = 0;
        virtual unsigned int execute(vector<Param> &bind) = 0;
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
        unsigned int execute(string sql);
        Statement prepare(string sql);
        Statement operator<<(string sql);
        bool begin();
        bool commit();
        bool rollback();
        bool begin(string name);
        bool commit(string name);
        bool rollback(string name);
        void* call(string name, void*);
        bool close();
        vector<string>& transactions();
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
        unsigned int rows();
        void bind(long v);
        void bind(double v);
        void bind(Param v);
        unsigned int execute();
        unsigned int execute(ResultRow &bind);
        Statement& operator<<(string sql);
        Statement& operator,(string v);
        Statement& operator%(string v);
        Statement& operator,(long v);
        Statement& operator%(long v);
        Statement& operator,(double v);
        Statement& operator%(double v);
        Statement& operator,(dbi::null const &e);
        Statement& operator%(dbi::null const &e);
        unsigned int  operator,(dbi::execute const &);
        ResultRow& fetchRow();
        ResultRowHash& fetchRowHash();
        unsigned int columns();
        vector<string> fields();
        unsigned char* fetchValue(unsigned int r, unsigned int c, unsigned long*);
        unsigned char* operator()(unsigned int r, unsigned int c);
        unsigned int currentRow();
        void advanceRow();
        bool finish();
        unsigned long lastInsertID();
    };

    bool dbiInitialize(string path);
    void dbiShutdown();

    bool trace();
    void trace(bool flag);
    void trace(bool flag, int fd);
    void logMessage(int fd, string msg);

    string generateCompactUUID();
}

#endif
