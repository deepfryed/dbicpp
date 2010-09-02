#ifndef _DBICXX_ERROR_H
#define _DBICXX_ERROR_H

#include <cxxabi.h>
#include <exception>

namespace dbi {

    using namespace std;

    class Error : public exception {
        public:
        Error(const char*);
        const char* what() const throw();
        ~Error() throw();
        protected:
        string message;
        char messagebuffer[8192];
    };

    class ConnectionError : public Error {
        public:
        ConnectionError(const char*);
        ConnectionError(string msg);
    };

    class RuntimeError : public Error {
        public:
        RuntimeError(const char*);
        RuntimeError(string msg);
    };

    class InvalidDriverError : public Error {
        public:
        InvalidDriverError(const char*);
        InvalidDriverError(string msg);
    };
}

#endif
