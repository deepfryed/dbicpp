DBICPP_SRC=src/dbicpp.cc
PQ_SRC=src/pq.cc
MYSQL_SRC=src/mysql.cc
MYSQLPP_SRC=src/mysql++.cc

DBICPP_EXEC=bin/dbicpp
PQ_EXEC=bin/pq
MYSQL_EXEC=bin/mysql
MYSQLPP_EXEC=bin/mysql++


all: $(DBICPP_EXEC) $(PQ_EXEC) $(MYSQL_EXEC) $(MYSQLPP_EXEC)

$(DBICPP_EXEC) : $(DBICPP_SRC) Makefile
	g++ -O3 -o $(DBICPP_EXEC) -rdynamic -I../inc -L../lib $(DBICPP_SRC) -lpcrecpp -luuid -ldl -ldbic++ -lpq -levent

$(PQ_EXEC) : $(PQ_SRC) Makefile
	g++ -O3 -o $(PQ_EXEC) $(PQ_SRC) -I /usr/include/postgresql -lpq -lpcrecpp

$(MYSQL_EXEC) : $(MYSQL_SRC) Makefile
	g++ -O3 -o $(MYSQL_EXEC) $(MYSQL_SRC) -lmysqlclient

$(MYSQLPP_EXEC) : $(MYSQLPP_SRC) Makefile
	g++ -O3 -I/usr/include/mysql -o $(MYSQLPP_EXEC) $(MYSQLPP_SRC) -lmysqlpp -lpcrecpp

clean: rm all

rm:
	rm -f bin/*
