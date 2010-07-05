DBICPP=dbicpp
PQEXEC=pq

all: $(DBICPP) $(PQEXEC)

$(DBICPP) : $(DBICPP).cc Makefile
	g++ -O3 -o $(DBICPP) -rdynamic -I../inc -L../libs $(DBICPP).cc -lpcrecpp -luuid -ldl -ldbic++ -lpq

$(PQEXEC) : $(PQEXEC).cc Makefile
	g++ -O3 -o $(PQEXEC) $(PQEXEC).cc -I /usr/include/postgresql -lpq -lpcrecpp

clean: rm all
	./insert_data.rb mysql 50
	./insert_data.rb postgresql 50

rm:
	rm -f pq dbicpp
