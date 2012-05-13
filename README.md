C++ database interface
======================

dbic++ is a database client library written in C++ which comes with support for the following databases,

* PostgreSQL >= 8.3
* MySQL      >= 5.0
* SQLite     >= 3.7

## MAIN FEATURES

* Simple and fast.
* Support for PostgreSQL, MySQL and SQLite3.
* Nested transactions.
* API for async queries (PostgreSQL and MySQL).

## EXAMPLES

### C++

```c++
  #include "dbic++.h"
  #include <unistd.h>

  using namespace std;
  using namespace dbi;

  int main() {

      // Handle h ("driver", "user", "password", "database", "host", "port");
      Handle h ("postgresql", dbi::getlogin(), "", "dbicpp");

      Query query (h, "SELECT id, name, email FROM users WHERE id >= ? AND id < ?");

      // bind and execute the statement.
      query % 1L, 10L;
      query.execute();

      ResultRow r;
      while (query.read(r))
          cout << r.join("\t") << endl;

      // or you can do
      query.rewind();
      ResultRowHash rh;
      while (query.read(rh))
          cout << rh["id"]    << "\t"
               << rh["name"]  << "\t"
               << rh["email"] << endl;

      query.finish();
  }
```

See src/examples for more specific examples.

### Ruby

You need to install swift (http://github.com/shanna/swift), it includes ruby bindings for
dbic++.

## INSTALL

### Dependencies

If you are building from source you need to install a few dependencies before
you compile dbic++. To begin with you need a working build environment with a
c++ compiler (XCode if you are a MacOSX user).

#### Most flavors of unix.

* cmake
* pcre3 development libraries
* uuid development libraries
* mysql client libraries (optional)
* postgresql client libraries (optional)
* sqlite3 development libraries (optional)

#### Debian

```
  sudo apt-get install build-essential
  sudo apt-get install cmake libpcre3-dev uuid-dev
  sudo apt-get install libmysqlclient-dev libpq-dev libsqlite3-dev
```

#### MacOSX

I'm assuming you're using homebrew. If not, please go to 
https://github.com/mxcl/homebrew/wiki/installation

You need to install the required dependencies first,

```
  brew install cmake
  brew install pcre
  brew install ossp-uuid
  brew install postgresql
  brew install mysql
  brew install sqlite3
```

The last three database are optional. You only need to install the ones you want to test or use.

### Building libraries and demos

```
  ./build
```

### System wide install of libraries

```
  sudo ./build -i
```

### Cleanup and uninstall

```
  ./build -c
  sudo ./build -u
```

### Building Debian packages

If you need to build debian packages yourself, you may need the following in addition.

```
  sudo apt-get install cdbs debhelper devscripts
```

To build debian packages for your local architecture,

```
  ./build -d
```

If you are too lazy to compile your own binaries you can use the ubuntu ppa at
https://launchpad.net/~deepfryed/+archive/ppa.

```
  sudo add-apt-repository ppa:deepfryed/dbic++
  sudo apt-get update
  sudo apt-get install dbic++-dev dbic++-pg dbic++-sqlite3 dbic++-mysql
```

### Running the demo

A few c++ examples can be found under src/examples/ and once you finish building the
binaries can be found under demo/

```
  ./demo/demo  [mysql|postgresql|sqlite3]
  ./demo/async [mysql|postgresql]
```

### Populating MySQL time zone tables.

This is not mandatory but would allow you to set timezones using Handle#setTimeZone
method in dbic++.

On most unix systems it can be done as,

```
  mysql_tzinfo_to_sql /usr/share/zoneinfo | mysql -u root mysql
```

Refer to http://dev.mysql.com/doc/refman/5.1/en/mysql-tzinfo-to-sql.html.

## BENCHMARKS

You can run these yourself if you're keen. The makefile might need some tweaks
on non-debian based distributions. The following results were obtained on a
Core2DUO 2.53GHz, 4GB, 5200rpm drive with stock database configs.

```
  * The dataset size is 50 rows of 3 fields (id, text, timestamp)
  * 10000 SELECT queries run back to back, fetching all rows and printing them.

    cd bench/ && make

    ./benchmarks.sh

    + Setting up sample data (50 rows)
      * mysql
      * postgresql
    + Done

    ./benchmarks.sh -w bench -n10000

    + Benchmarking mysql

      * mysql      user 0.69 sys 0.21 real 2.34
      * mysql++    user 1.06 sys 0.09 real 2.74
      * dbic++     user 0.81 sys 0.25 real 2.58

    + Benchmarking postgresql

      * pq         user 0.42 sys 0.14 real 2.01
      * dbic++     user 0.49 sys 0.12 real 2.05

    + Finished
```

## TODO

* sqlite3: Simulate async IO operations.
* mysql: Proper bind parameter interpolation for Handle::execute(string, vector<Param>&)
* Cursor support, generic interface to database specific api.

## GOTCHAS

* sqlite3: No support for async operations yet.
* mysql: Handle::execute(...) replaces ? with given bind parameters if bind parameters are provided.
  Any literal values with ? character will be interpreted as a bind argument. If you are using
  Handle::execute(...) with bind arguments, make sure to provide all values as bind parameters to
  avoid the confusion and resulting errors.

## LICENSE

[Creative Commons Attribution - CC BY](http://creativecommons.org/licenses/by/3.0)

## WARNING

Like any other OSS, do your research and do your testing (especially if you're using it in space shuttles).
