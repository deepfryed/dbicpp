AR=ar
CC=g++
SRCDIR=src
OBJDIR=objs
LIBDIR=libs
INCDIR=inc
CFLAGS=-ggdb -Wunused-function -Wunused-value -Wunused-label -I$(INCDIR) -O3 -fPIC

DBILIB_SFILES=dbic++.cc container.cc param.cc
DBILIB_OFILES=$(DBILIB_SFILES:.cc=.o)
DBILIB_SOURCES=$(patsubst %,$(SRCDIR)/%,$(DBILIB_SFILES))
DBILIB_OBJECTS=$(patsubst %,$(OBJDIR)/%,$(DBILIB_OFILES))

EXE_SOURCES=example.cc
EXE_OBJECTS=$(EXE_SOURCES:.cc=.o)

EXE=example
DBILIB=$(LIBDIR)/libdbic++.a

INCLUDES=$(INCDIR)/*.h $(INCDIR)/*/*.h

#-------------------------------------------------------------------------

DRVDIR=src/drivers

DBD_PGVMAJOR=1
DBD_PGVMINOR=1.0.1

DBD_PGSOURCES=$(DRVDIR)/pg.cc
DBD_PGOBJECTS=$(DRVDIR)/pg.o

DBD_PGSOFILE=$(LIBDIR)/libdbdpg.so.$(DBD_PGVMINOR)
DBD_PGSONAME=libdbdpg.so.$(DBD_PGVMAJOR)

DBD_PGLDFLAGS=-L$(LIBDIR) -lpq

DBD_MYVMAJOR=1
DBD_MYVMINOR=1.0.1

DBD_MYSOURCES=$(DRVDIR)/mysql.cc
DBD_MYOBJECTS=$(DRVDIR)/mysql.o

DBD_MYSOFILE=$(LIBDIR)/libdbdmysql.so.$(DBD_MYVMINOR)
DBD_MYSONAME=libdbdmysql.so.$(DBD_MYVMAJOR)

DBD_MYLDFLAGS=-L$(LIBDIR) -lmysqlclient

DBD_CFLAGS=-I/usr/include/mysql -I/usr/include/postgresql

#-------------------------------------------------------------------------

LDFLAGS=-L$(LIBDIR) -ldl -lpcrecpp -luuid

all: $(DBILIB) $(EXE) $(DBD_PGSOFILE) $(DBD_MYSOFILE)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc Makefile $(INCLUES)
	$(CC) -c $(CFLAGS) $< -o $@

$(DRVDIR)/%.o: $(DRVDIR)/%.cc Makefile $(INCLUDES)
	$(CC) -fPIC -c $(CFLAGS) $(DBD_CFLAGS) $< -o $@

$(DBILIB): $(DBILIB_OBJECTS) $(DBILIB_SOURCES)
	$(AR) rcs $(DBILIB) $(DBILIB_OBJECTS)

$(DBD_PGSOFILE): $(DBD_PGOBJECTS) $(DBILIB) Makefile
	$(CC) -shared -Wl,-soname,$(DBD_PGSONAME) $(DBD_PGLDFLAGS) -o $(DBD_PGSOFILE) $(DBD_PGOBJECTS)

$(DBD_MYSOFILE): $(DBD_MYOBJECTS) $(DBILIB) Makefile
	$(CC) -shared -Wl,-soname,$(DBD_MYSONAME) $(DBD_MYLDFLAGS) -o $(DBD_MYSOFILE) $(DBD_MYOBJECTS)

$(EXE): $(SRCDIR)/example.cc $(DBILIB)
	$(CC) $(CFLAGS) -rdynamic $(SRCDIR)/example.cc -o $@ $(LDFLAGS) -ldbic++ $(DBD_PGLDFLAGS) $(DBD_MYLDFLAGS)

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.so.* $(OBJDIR)/*.a $(EXE) $(DRVDIR)/*.o
