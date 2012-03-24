#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

namespace dbi {
    char *getlogin() {
        struct passwd *ptr = getpwuid(getuid());
        return ptr ? ptr->pw_name : getenv("USER");
    }
}
