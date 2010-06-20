DBICPP=dbicpp-test
PQEXEC=pq-test
ITER=25000

bench: $(DBICPP) $(PQEXEC)
	@echo -n "Ruby PG "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./rubypg-test.rb $(ITER) > /dev/null
	@echo -n "C++ lib "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./dbicpp-test    $(ITER) > /dev/null
	@echo -n "C libpq "; time -f 'cpu:%P sys:%S user:%U wall:%E' ./pq-test        $(ITER) > /dev/null

$(DBICPP) : $(DBICPP).cc
	g++ -O3 -o $(DBICPP) -rdynamic -I../inc -L../libs $(DBICPP).cc -lpcrecpp -ldl -ldbic++ -lpq -luuid

$(PQEXEC) : $(PQEXEC).c
	g++ -O3 -o $(PQEXEC) $(PQEXEC).c -I /usr/include/postgresql -lpq
