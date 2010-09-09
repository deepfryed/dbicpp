#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

namespace dbi {
    using namespace std;

    struct ProcNet {
        ino_t inode;
        struct in_addr local_addr, remote_addr;
        u_int local_port, remote_port;
        u_char status, protocol;
        uid_t uid;
        int fd;
    };

    class SocketInfo {
        protected:
        struct in_addr remotehost;
        u_int  remoteport;
        pid_t  pid;


        int total;
        char buffer[128];
        ProcNet *net_data;

        void read_net_info();

        public:
        static int compare(const void *a, const void *b);
        SocketInfo(string host, u_int port, pid_t pid);
        SocketInfo(pid_t pid);
        ~SocketInfo();
        vector<ProcNet*> search();
    };
}
