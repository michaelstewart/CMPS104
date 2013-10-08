#include <string>
using namespace std;

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "auxlib.h"

const string* intern_stringset (const char*);

void dump_stringset (FILE*);

void cpplines (FILE *pipe, char *filename);


void chomp (char *string, char delim);