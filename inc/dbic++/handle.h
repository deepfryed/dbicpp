namespace dbi {

    using namespace std;
    using namespace pcrecpp;

    class Result;
    class Statement;

    /*
        Class: Handle
        Facade for AbstractHandle. This is the class that you should normally use.

        Example:

        (begin code)

        #include "dbic++.h"
        #include <unistd.h>

        using namespace std;
        using namespace dbi;

        int main(int argc, char *argv[]) {
            ResultRowHash res;

            Handle h ("mysql", getlogin(), "", "dbicpp");

            // Set trace on and log queries to stderr
            trace(true, fileno(stderr));

            Query query (h, "select id, name from users where name like ? limit 5");

            query % "james%", execute();

            while (query.read(res)) {
                cout << "id: "   << res["id"]
                     << "name: " << res["name"]
                     << endl;
            }
        }

        (end)
    */
    class Handle {
        protected:
        vector<string> trx;
        AbstractHandle *h;
        public:
        /*
            Constructor: Handle(string, string, string, string, string, string)
            Parameters:
                driver - driver name (postgresql, mysql, etc.)
                user   - database user name
                pass   - database password, can be empty string.
                dbname - database name.
                host   - host name or ip address (socket based connections are not supported at this time).
                port   - port number as a string.
        */
        Handle(string driver, string user, string pass, string dbname, string host, string port);

        /*
            Constructor: Handle(string, string, string, string)
            Parameters:
            driver - driver name (postgresql, mysql, etc.)
            user   - database user name
            pass   - database password, can be empty string.
            dbname - database name.

            This assumes localhost and default port number for the database.
        */
        Handle(string driver, string user, string pass, string dbname);

        /*
            Constructor: Handle(AbstractHandle*)
            Parameters:
            handle - An abstract handle if you ended up creating one directly.
        */
        Handle(AbstractHandle *handle);

        /*
            Function: conn
            You need to use this to access the underlying database handle. In most cases
            you don't need this. It might be useful if you are writing a dbic++ binding
            in another language and need the underlying handle.

            Returns:
                handle - The abstract handle wrapped inside.
        */
        AbstractHandle* conn();

        ~Handle();

        /*
            Function: execute(string)
            See <AbstractHandle::execute(string)>
        */
        uint32_t execute(string sql);

        /*
            Function: execute(string, vector<Param>&)
            See <AbstractHandle::execute(string, vector<Param>&)>
        */
        uint32_t execute(string sql, vector<Param> &bind);

        /*
            Function: result
            Returns a pointer to a result object. This needs to be
            deallocated explicitly.

            Returns:
            Result* - Pointer to the Result set object.
        */
        Result* result();

        /*
            Function: prepare(string)
            Prepares a SQL and returns a statement handle.

            Parameters:
            sql - SQL statement.

            Returns:
            A pointer to Statement.
        */
        Statement* prepare(string sql);

        /*
            Operator: <<(string)
            This is an alias for prepare.

            Parameters:
            sql - SQL statement.

            Returns:
            A pointer to Statement.
        */
        Statement* operator<<(string sql);

        /*
            Function: begin
            See <AbstractHandle::begin()>
        */
        bool begin();

        /*
            Function: commit
            See <AbstractHandle::commit()>
        */
        bool commit();

        /*
            Function: rollback
            See <AbstractHandle::rollback()>
        */
        bool rollback();

        /*
            Function: begin(string)
            See <AbstractHandle::begin(string)>
        */
        bool begin(string name)
;
        /*
            Function: commit(string)
            See <AbstractHandle::commit(string)>
        */
        bool commit(string name);

        /*
            Function: rollback(string)
            See <AbstractHandle::rollback(string)>
        */
        bool rollback(string name);

        /*
            Function: close
            Close connection.

            Returns:
            true or false - denotes success or failure.
        */
        bool close();

        /*
            Function: transactions
            Returns a list of transactions in the transaction stack.

            Returns:
            list of transactions.
        */
        vector<string>& transactions();

        /*
            Function: write(string, FieldSet&, IO*)
            See <AbstractHandle::write(string, FieldSet&, IO*)>
        */
        uint64_t write(string table, FieldSet &fields, IO*);

        /*
            Function: setTimeZoneOffset(int, int)
            See <AbstractHandle::setTimeZoneOffset(int, int)>
        */
        void setTimeZoneOffset(int hours, int mins);

        /*
            Function: setTimeZone(char *)
            See <AbstractHandle::setTimeZone(char*)>
        */
        void setTimeZone(char *zone);

        /*
            Function: escape(string)
            See <AbstractHandle::escape(string)>
        */
        string escape(string);

        /*
            Function: driver
            See <AbstractHandle::driver()>
        */
        string driver();

        /*
            Function: reconnect
            See <AbstractHandle::reconnect()>
        */
        void reconnect();

        friend class Statement;
        friend class Query;
    };
}


