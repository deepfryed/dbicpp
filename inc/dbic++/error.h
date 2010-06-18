#ifndef _DBICXX_ERROR_H
#define _DBICXX_ERROR_H

#include <cxxabi.h>
#include <exception>

namespace dbi {

    using namespace std;

    class Error : public exception {
        public:
        Error(const char* msg = "UNKNOWN") : exception() { message = string(msg); }
        const char* what() const throw() {
            char *demangled_klass = __cxxabiv1::__cxa_demangle(typeid(*this).name(), 0, 0, 0);
            string klass = string(demangled_klass);
            delete demangled_klass;
            return ("[ " + klass + " ] " + message).c_str();
        }
        ~Error() throw() {}
        protected:
        string message;
    };

    class ConnectionError : public Error {
        public:
        ConnectionError(const char* msg = "UNKNOWN") : Error(msg) {}
        ConnectionError(string msg) : Error(msg.c_str()) {}
    };

    class RuntimeError : public Error {
        public:
        RuntimeError(const char* msg = "UNKNOWN") : Error(msg) {}
        RuntimeError(string msg) : Error(msg.c_str()) {}
    };

    class InvalidDriverError : public Error {
        public:
        InvalidDriverError(const char* msg = "UNKNOWN") : Error(msg) {}
        InvalidDriverError(string msg) : Error(msg.c_str()) {}
    };
}

#endif
