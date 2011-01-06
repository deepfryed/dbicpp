#!/bin/bash

rows=50
runs=5000
work=setup
mysql_query="select SQL_NO_CACHE id, name, created_at from users where id > ? and created_at < now()"
pgsql_query="select id, name, created_at from users where id > ? and created_at < now()"

dir=$(readlink -f $(dirname $0))
export DBICPP_LIBDIR=$dir/../lib/dbic++

setup() {
  echo "+ Setting up sample data ($rows rows)"

  exe=$dir/bin/dbicpp
  for driver in mysql postgresql; do
    echo "  * $driver"
    $exe -d $driver -s "drop table if exists users"
    $exe -d $driver -s "create table users(id serial, name text, created_at timestamp, primary key(id))"
    $exe -d $driver -s "insert into users(name, created_at) values(?, now())" -b "Jimmy James" -n $rows > /dev/null
  done

  echo "+ Done"
}

benchmark() {
  timer=/usr/bin/time
  echo
  echo "+ Benchmarking mysql"
  echo
  for file in mysql mysql++ dbicpp; do
    stats=`$timer -f "user %U sys %S real %e" $dir/bin/$file -d mysql -b 1 -s "$mysql_query" -n $runs -o /dev/null 2>&1`
    label=$(echo $file | sed 's/dbicpp/dbic++/')
    printf "  * %-10s %s" $label "$stats"
    echo
  done

  echo
  echo "+ Benchmarking postgresql"
  echo
  for file in pq dbicpp; do
    stats=`$timer -f "user %U sys %S real %e" $dir/bin/$file -d postgresql -b 1 -s "$pgsql_query" -n $runs -o /dev/null 2>&1`
    label=$(echo $file | sed 's/dbicpp/dbic++/')
    printf "  * %-10s %s" $label "$stats"
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
    -w [setup|run]
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
    run) benchmark;;
      *) usage; exit 1;;
esac
