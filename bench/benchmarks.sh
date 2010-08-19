#!/bin/bash

rows=50
runs=5000
work=setup
mysql_query="select SQL_NO_CACHE id, name, created_at from users where id > 1 and created_at < now()"
pgsql_query="select id, name, created_at from users where id > 1 and created_at < now()"


setup() {
  echo "+ Setting up sample data ($rows rows)"

  for driver in mysql postgresql; do
    echo "  * $driver"
    ./bin/dbicpp -d $driver -o /dev/null -s "drop table if exists users"
    ./bin/dbicpp -d $driver -o /dev/null -s "create table users(id serial, name text, created_at timestamp, primary key(id))"
    ./bin/dbicpp -d $driver -o /dev/null -s "insert into users(name, created_at) values(?, now())" -b "Jimmy James" -n $rows
  done

  echo "+ Done"
}

benchmark() {
  echo
  echo "+ Benchmarking mysql"
  echo
  for file in mysql mysql++ dbicpp; do
    stats=`/usr/bin/time -f "user %U sys %S real %e" ./bin/$file -d mysql -s "$mysql_query" -n $runs -o /dev/null 2>&1`
    printf "  * %-10s %s" $file "$stats"
    echo
  done

  echo
  echo "+ Benchmarking postgresql"
  echo
  for file in pq dbicpp; do
    stats=`/usr/bin/time -f "user %U sys %S real %e" ./bin/$file -s "$pgsql_query" -n $runs -o /dev/null 2>&1`
    printf "  * %-10s %s" $file "$stats"
    echo
  done

  echo
  echo "+ Finished"
}

usage() {
  echo "

    $0 [options]

    -h print this help message.
    -r inserts this many rows into database (default 50)
    -n number of iterations (default 5000)
    -w [setup|bench]
  "
}

while getopts "hr:n:w:" OPTION
do
  case $OPTION in
    h) usage; exit 0;;
    r) rows=$OPTARG;;
    n) runs=$OPTARG;;
    w) work=$OPTARG;;
    *) usage; exit 1;;
  esac
done

case $work in
  setup) setup;;
  bench) benchmark;;
  *)     usage; exit 1;;
esac
