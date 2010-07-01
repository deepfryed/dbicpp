DBICPP=dbicpp-test
PQEXEC=pq-test
ITER=20000

bench: $(DBICPP) $(PQEXEC)
	@echo -n "C libpq    "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./pq-test $(ITER) > /dev/null
	@echo -n "Ruby libpq "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./pg-test.rb $(ITER) > /dev/null
	@echo -n "DBI++      "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./dbicpp-test $(ITER) > /dev/null
	@echo -n "Ruby DBI++ "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./dbicpp-test.rb $(ITER) > /dev/null

$(DBICPP) : $(DBICPP).cc
	g++ -O3 -o $(DBICPP) -rdynamic -I../inc -L../libs $(DBICPP).cc -lpcrecpp -luuid -ldl -ldbic++

$(PQEXEC) : $(PQEXEC).c
	g++ -O3 -o $(PQEXEC) $(PQEXEC).c -I /usr/include/postgresql -lpq

clean:
	rm -f pq-test dbicpp-test
