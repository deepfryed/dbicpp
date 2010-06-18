AR=ar
CC=g++
SRCDIR=src
OBJDIR=objs
LIBDIR=libs
INCDIR=inc
CFLAGS=-ggdb -Wall -I$(INCDIR)

DBILIB_SFILES=dbic++.cc
DBILIB_OFILES=$(DBILIB_SFILES:.cc=.o)
DBILIB_SOURCES=$(patsubst %,$(SRCDIR)/%,$(DBILIB_SFILES))
DBILIB_OBJECTS=$(patsubst %,$(OBJDIR)/%,$(DBILIB_OFILES))

EXE_SOURCES=example.cc
EXE_OBJECTS=$(EXE_SOURCES:.cc=.o)

EXE=example
DBILIB=$(LIBDIR)/libdbic++.a

INCLUDES=$(INCDIR)/*.h $(INCDIR)/*/*.h

#-------------------------------------------------------------------------

DRVDIR=drivers
DBD_PGVMAJOR=1
DBD_PGVMINOR=1.0.1 

DBD_PGSOURCES=$(DRVDIR)/pg.cc $(DRVDIR)/container.cc
DBD_PGOBJECTS=$(DRVDIR)/pg.o $(DRVDIR)/container.o

DBD_PGSOFILE=$(LIBDIR)/libdbdpg.so.$(DBD_PGVMINOR)
DBD_PGSONAME=libdbdpg.so.$(DBD_PGVMAJOR)

DBD_PGLDFLAGS=-L$(LIBDIR) -lpcrecpp -lpq -luuid
DBD_CFLAGS=-I/usr/include/postgresql -I/usr/include/postgresql/8.4/server 

#-------------------------------------------------------------------------

LDFLAGS=-L$(LIBDIR) -ldl -lpcrecpp

all: $(DBILIB) $(EXE) $(DBD_PGSOFILE)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc Makefile $(INCLUES)
	$(CC) -c $(CFLAGS) $< -o $@

$(DRVDIR)/%.o: $(DRVDIR)/%.cc Makefile $(INCLUDES)
	$(CC) -fPIC -c $(CFLAGS) $(DBD_CFLAGS) $< -o $@

$(DBILIB): $(DBILIB_OBJECTS) $(DBILIB_SOURCES)
	$(AR) rcs $(DBILIB) $(DBILIB_OBJECTS)

$(DBD_PGSOFILE): $(DBD_PGOBJECTS) Makefile
	$(CC) -shared -Wl,-soname,$(DBD_PGSONAME) $(DBD_PGLDFLAGS) -o $(DBD_PGSOFILE) $(DBD_PGOBJECTS) 

$(EXE): example.cc $(DBILIB)
	$(CC) $(CFLAGS) example.cc -o $@ $(LDFLAGS) -ldbic++

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.so.* $(OBJDIR)/*.a $(EXE)
