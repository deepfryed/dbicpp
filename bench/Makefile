DBICPP_SRC=src/dbicpp.cc
PQ_SRC=src/pq.cc

DBICPP_EXEC=bin/dbicpp
PQ_EXEC=bin/pq

all: $(DBICPP_EXEC) $(PQ_EXEC)

$(DBICPP_EXEC) : $(DBICPP_SRC) Makefile
	g++ -O3 -o $(DBICPP_EXEC) -rdynamic -I../inc -L../lib $(DBICPP_SRC) -lpcrecpp -luuid -ldl -ldbic++ -lpq

$(PQ_EXEC) : $(PQEXEC_SRC) Makefile
	g++ -O3 -o $(PQ_EXEC) $(PQ_SRC) -I /usr/include/postgresql -lpq -lpcrecpp

clean: rm all
	./setup.rb mysql 50
	./setup.rb postgresql 50

rm:
	rm -f bin/*
