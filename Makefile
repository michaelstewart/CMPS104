# $Id: Makefile,v 1.5 2013-09-25 13:51:12-07 - - $

GCC        = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS     = g++ -MM -std=gnu++0x

MKFILE     = Makefile
DEPSFILE   = Makefile.deps
SOURCES    = auxlib.cc stringset.cc oc.cc
HEADERS    = auxlib.h stringset.h
OBJECTS    = ${SOURCES:.cc=.o}
EXECBIN    = oc
SRCFILES   = ${HEADERS} ${SOURCES} ${MKFILE}
CHECKINS   = ${SRCFILES}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

%.o : %.cc
	${GCC} -c $<

ci :
	git add ${CHECKINS}

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


