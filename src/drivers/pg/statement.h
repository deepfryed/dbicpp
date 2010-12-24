#include "src/drivers/pg/common.h"

namespace dbi {
    class PgStatement : public AbstractStatement {
        private:
        string _sql, _uuid;
        bool   _async;

        PGresult* _pgexec();
        PGresult* _pgexec(vector<Param>&);

        protected:
        PgHandle *handle;
        void      init();
        PGresult* prepare();
        void      boom(const char *);

        public:
        PgStatement();
        ~PgStatement();
        PgStatement(string query, PGconn *conn);
        void cleanup();
        string command();

        uint32_t  execute();
        uint32_t  execute(vector<Param>&);
        PgResult* query();
        PgResult* query(vector<Param>&);
    };
}
