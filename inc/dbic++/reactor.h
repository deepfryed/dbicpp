#ifndef _DBICXX_REACTOR_H
#define _DBICXX_REACTOR_H

#include "dbic++.h"
#include <event.h>

namespace dbi {
    static bool _reactorInitialized;
    class Reactor {
        public:
        static void run();
        static void stop();
        static void initialize();
        static void watch(Request *);
        static void callback(int fd, short type, void *arg);
    };
}

#endif
