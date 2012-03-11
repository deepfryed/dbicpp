namespace dbi {

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
        string_list_t trx;
        AbstractHandle *h;
        public:
        /*
            Constructor: Handle(string, string, string, string, string, string, char*)
            Parameters:
                driver  - driver name (postgresql, mysql, etc.)
                user    - database user name
                pass    - database password, can be empty string.
                dbname  - database name.
                host    - host name or ip address (socket based connections are not supported at this time).
                port    - port number as a string.
                options - driver specific options as a string, only ssl specific options are supported at present.
                  e.g.
                    "sslca=$HOME/certs/ca-cert.pem"
                    "sslcert=$HOME/certs/client-cert.pem;sslkey=$HOME/certs/client-key.pem"

                  The mysql driver supports the following settings at present,
                    sslca, sslcert, sslkey, sslcapath, sslcipher

                  The postgresql driver can be set to use client certificates using the following settings,
                    sslcert, sslkey

                  sqlite3 driver ignores these options currently.



        */
        Handle(std::string driver, std::string user, std::string pass, std::string dbname,
               std::string host, std::string port, char *options = 0);

        /*
            Constructor: Handle(string, string, string, string)
            Parameters:
            driver - driver name (postgresql, mysql, etc.)
            user   - database user name
            pass   - database password, can be empty string.
            dbname - database name.

            This assumes localhost and default port number for the database.
        */
        Handle(std::string driver, std::string user, std::string pass, std::string dbname);

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
        uint32_t execute(std::string sql);

        /*
            Function: execute(string, param_list_t&)
            See <AbstractHandle::execute(string, param_list_t&)>
        */
        uint32_t execute(std::string sql, param_list_t &bind);


        Result* aexecute(std::string sql);
        Result* aexecute(std::string sql, param_list_t &bind);

        int socket();

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
        Statement* prepare(std::string sql);

        /*
            Operator: <<(string)
            This is an alias for prepare.

            Parameters:
            sql - SQL statement.

            Returns:
            A pointer to Statement.
        */
        Statement* operator<<(std::string sql);

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
        bool begin(std::string name)
;
        /*
            Function: commit(string)
            See <AbstractHandle::commit(string)>
        */
        bool commit(std::string name);

        /*
            Function: rollback(string)
            See <AbstractHandle::rollback(string)>
        */
        bool rollback(std::string name);

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
        string_list_t& transactions();

        /*
            Function: write(string, FieldSet&, IO*)
            See <AbstractHandle::write(string, FieldSet&, IO*)>
        */
        uint64_t write(std::string table, field_list_t &fields, IO*);

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
        string escape(std::string);

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


