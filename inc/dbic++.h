#ifndef _DBICXX_BASE_H
#define _DBICXX_BASE_H

#include <iostream>
#include <string>
#include <cstdarg>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pcrecpp.h>

namespace dbi {
    struct null {};
    struct execute {};
}

#include "dbic++/error.h"
#include "dbic++/container.h"

#define DEFAULT_DRIVER_PATH "/usr/lib/dbi++"
#define OPTIONAL_ARG(a, v) a ? a : v

namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    class AbstractStatement;

    class AbstractHandle {
        public:
        AbstractHandle() {};
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
    };

    class AbstractStatement {
        public:
        AbstractStatement() {};
        virtual unsigned int rows() = 0;
        virtual unsigned int execute() = 0;
        virtual unsigned int execute(vector<Param> &bind) = 0;
        virtual ResultRow fetchRow() = 0;
        virtual ResultRowHash fetchRowHash() = 0;
        virtual bool finish() = 0;
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
        AbstractHandle *h;
        public:
        Handle(AbstractHandle *ah);
        ~Handle();
        unsigned int execute(string sql);
        unsigned int execute(string sql, vector<Param> &bind);
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
        friend class Statement;
    };

    class Statement {
        private:
        AbstractStatement *st;
        AbstractHandle *h;
        vector<Param> params;
        public:
        Statement();
        Statement(AbstractStatement *);
        Statement(Handle &h);
        Statement(Handle &h, string sql);
        ~Statement();
        unsigned int rows();
        void bind(long v);
        void bind(double v);
        void bind(Param v);
        unsigned int execute();
        unsigned int execute(vector<Param> &bind);
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
        ResultRow fetchRow();
        ResultRowHash fetchRowHash();
        bool finish();
    };

    Handle dbiConnect(string driver, string user, string pass, string dbname);
    Handle dbiConnect(string driver, string user, string pass, string dbname, string host);
    Handle dbiConnect(string driver, string user, string pass, string dbname, string host, string port);

    bool dbiInitialize(string path);

    void dbiShutdown();
}

#endif
