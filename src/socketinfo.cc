#include "dbic++.h"

/*
    Mostly based on:

    SocketStat v1.0 - by Richard Steenbergen <humble@lightning.net> and
    Drago <Drago@Drago.com>. Inspired by dreams, coded by nightmares.
*/

namespace dbi {
    using namespace std;
    SocketInfo::SocketInfo(string host, u_int port, pid_t pid) {
        struct hostent *ptr = gethostbyname(host.c_str());
        if (!ptr)
            throw RuntimeError("Unable to resolve hostname:" + host);

        memcpy(&remotehost, ptr->h_addr_list[0], sizeof(remotehost));

        this->remoteport = port;
        this->net_data   = 0;
        this->pid        = pid;
        read_net_info();
    }

    SocketInfo::SocketInfo(pid_t pid) {
        this->remoteport = 0;
        this->net_data   = 0;
        this->pid        = pid;
        read_net_info();
    }

    SocketInfo::~SocketInfo() {
        if (net_data)
            free(net_data);
    }

    int SocketInfo::compare(const void*a, const void* b) {
        ProcNet *a_rec, *b_rec;

        a_rec = (ProcNet *) a;
        b_rec = (ProcNet *) b;

        if (a_rec->inode == b_rec->inode)
            return 0;
        else
            return (a_rec->inode > b_rec->inode)?(1):(-1);
    }

    void SocketInfo::read_net_info() {
        char filename[1024];
        unsigned int i = 0, size = 256;

        snprintf(filename, 1024, "/proc/%d/net/tcp", pid);
        FILE *fp = fopen(filename, "r");

        if (!fp)
            throw RuntimeError("Cannot open tcp net info");

        if ((net_data = (ProcNet*)calloc(sizeof(ProcNet), size)) == NULL)
            throw RuntimeError("Unable to allocate memory");

        while(fgets(buffer, sizeof(buffer), fp)) {
            if (i == size) {
                size *= 2;
                if ((net_data = (ProcNet*)realloc(net_data, (sizeof(ProcNet) * size))) == NULL)
                    throw RuntimeError("Unable to allocate memory");
            }

            if (sscanf(buffer, "%*u: %lX:%x %lX:%x %hx %*X:%*X %*x:%*X %*x %u %*u %u",
                (u_long  *)&net_data[i].local_addr,  (u_int *)&net_data[i].local_port,
                (u_long  *)&net_data[i].remote_addr, (u_int *)&net_data[i].remote_port,
                (u_short *)&net_data[i].status,      (u_int *)&net_data[i].uid,
                (u_int   *)&net_data[i].inode) != 7)
                continue;
            i++;
        }

        fclose(fp);

        total = i;
        qsort(net_data, total, sizeof(ProcNet), compare);
    }

    vector<ProcNet*> SocketInfo::search() {
        DIR *dir;
        ProcNet *ptr;
        struct stat st;
        struct dirent *fdent;
        vector<ProcNet*> results;

        snprintf(buffer, sizeof(buffer), "/proc/%d/fd/", pid);
        if ((dir = opendir(buffer)) == NULL)
            throw RuntimeError("Unable to open proc fd directory:" + string(buffer));

        while((fdent = readdir(dir)) != NULL) {
            snprintf(buffer, sizeof(buffer), "/proc/%d/fd/%s", pid, fdent->d_name);
            if (stat(buffer, &st) < 0)
               continue;
            if (!S_ISSOCK(st.st_mode))
               continue;
            if ((ptr = (ProcNet*)bsearch(&st.st_ino, net_data, total, sizeof(ProcNet), compare)) != NULL) {
                ptr->fd = atoi(fdent->d_name);
                if (remoteport > 0) {
                    if(ptr->remote_port == remoteport && memcmp(&ptr->remote_addr, &remotehost, sizeof(remotehost)) == 0)
                        results.push_back(ptr);
                }
                else
                    results.push_back(ptr);
            }
        }

        closedir(dir);
        return results;
    }
}
