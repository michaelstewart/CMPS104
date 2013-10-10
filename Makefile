# $Id: Makefile,v 1.5 2013-09-25 13:51:12-07 - - $

GCC        = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS     = g++ -MM -std=gnu++0x
VALGRIND   = valgrind --leak-check=full --show-reachable=yes

MKFILE     = Makefile
DEPSFILE   = Makefile.deps
SOURCES    = auxlib.cc cppstrtok.cc stringset.cc oc.cc
HEADERS    = auxlib.h cppstrtok.h stringset.h
OBJECTS    = ${SOURCES:.cc=.o}
EXECBIN    = oc
SRCFILES   = ${HEADERS} ${SOURCES} ${MKFILE}
SMALLFILES = ${DEPSFILE} foo.oc foo1.oh foo2.oh
CHECKINS   = ${SRCFILES} ${SMALLFILES}
LISTING    = Listing.ps

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

%.o : %.cc
	${GCC} -c $<

ci :
	cid + ${CHECKINS}
	checksource ${CHECKINS}

clean :
	- rm ${OBJECTS}

spotless : clean
	- rm ${EXECBIN}

${DEPSFILE} :
	${MKDEPS} ${SOURCES} >${DEPSFILE}

deps :
	- rm ${DEPSFILE}
	${MAKE} --no-print-directory ${DEPSFILE}

include Makefile.deps

test : ${EXECBIN}
	${VALGRIND} ${EXECBIN} foo.oc 1>test.out 2>test.err
	morecat ${SMALLFILES} test.out test.err >test.lis 2>&1
	rm test.out test.err


