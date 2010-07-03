DBICPP=dbicpp
PQEXEC=pq
ITER=20000

bench: $(DBICPP) $(PQEXEC)
	@echo -n "C libpq    "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./$(PQEXEC)    $(ITER) > /dev/null
	@echo -n "Ruby libpq "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./$(PQEXEC).rb $(ITER) > /dev/null
	@echo -n "DBI++      "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./$(DBICPP)    $(ITER) > /dev/null
	@echo -n "Ruby DBI++ "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./$(DBICPP).rb $(ITER) > /dev/null

$(DBICPP) : $(DBICPP).cc
	g++ -O3 -o $(DBICPP) -rdynamic -I../inc -L../libs $(DBICPP).cc -lpcrecpp -luuid -ldl -ldbic++

$(PQEXEC) : $(PQEXEC).c
	g++ -O3 -o $(PQEXEC) $(PQEXEC).c -I /usr/include/postgresql -lpq

clean:
	rm -f pq dbicpp
	./insert_data.rb
