// $Id: stringset.h,v 1.1 2013-09-20 19:45:47-07 - - $

#ifndef __STRINGSET__
#define __STRINGSET__

#include <string>
#include <unordered_set>
using namespace std;

#include <stdio.h>

const string* intern_stringset (const char* string, bool no_insert);

void dump_stringset (FILE*);

#endif
