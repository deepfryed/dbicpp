#include "dbic++.h"

namespace dbi {

    using namespace std;

    Error::Error(const char* msg = "UNKNOWN") : exception() {
        message = string(msg);
    }

    const char* Error::what() const throw() {
        char *demangled_klass = __cxxabiv1::__cxa_demangle(typeid(*this).name(), 0, 0, 0);
        string klass = string(demangled_klass);
        delete demangled_klass;
        snprintf((char*)msgbuffer, 8192, "%s", ("[ " + klass + " ] " + message).c_str());
        return (const char *)msgbuffer;
    }

    Error::~Error() throw() {}

    ConnectionError::ConnectionError(const char* msg = "UNKNOWN") : Error(msg) {}
    ConnectionError::ConnectionError(string msg) : Error(msg.c_str()) {}

    RuntimeError::RuntimeError(const char* msg = "UNKNOWN") : Error(msg) {}
    RuntimeError::RuntimeError(string msg) : Error(msg.c_str()) {}

    InvalidDriverError::InvalidDriverError(const char* msg = "UNKNOWN") : Error(msg) {}
    InvalidDriverError::InvalidDriverError(string msg) : Error(msg.c_str()) {}
}
