#!/bin/bash

rows=${1:-"50"}
runs=${2:-"100"}
mysql_query="select SQL_NO_CACHE id, name, created_at from users"
pgsql_query="select id, name, created_at from users"

echo "+ Setting up sample data ($rows rows)"

for driver in mysql postgresql; do
  echo "  * $driver"
  ./bin/dbicpp -d $driver -o /dev/null -s "drop table if exists users"
  ./bin/dbicpp -d $driver -o /dev/null -s "create table users(id serial, name text, created_at timestamp, primary key(id))"
  ./bin/dbicpp -d $driver -o /dev/null -s "insert into users(name, created_at) values(?, now())" -b "Jimmy James" -n $rows
done

echo "+ Done"
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
