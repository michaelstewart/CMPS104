// $Id: main.cc,v 1.3 2013-09-23 14:39:10-07 - - $

#include <string>
using namespace std;

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "auxlib.h"
#include "stringset.h"
#include "cppstrtok.h"

const string CPP = "/usr/bin/cpp";

int main (int argc, char **argv) {
   set_execname (argv[0]);
   for (int argi = 1; argi < argc; ++argi) {
      char *filename = argv[argi];
      string command = CPP + " " + filename;
      // printf ("command=\"%s\"\n", command.c_str());
      FILE *pipe = popen (command.c_str(), "r");
      if (pipe == NULL) {
         syserrprintf (command.c_str());
      }else {
         cpplines (pipe, filename);
         int pclose_rc = pclose (pipe);
         eprint_status (command.c_str(), pclose_rc);
      }
   }

   printf("========================\n");
   dump_stringset (stdout);

   return get_exitstatus();
}

