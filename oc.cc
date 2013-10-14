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

string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;
const string OPT_STRING = "-lyD:@:";
int option;
int file_arg_i = 0;

// Run cpp against the lines of the file.
void cpplines (FILE *pipe) {
   for (;;) {
      char buffer[LINESIZE];
      // Read a line in from the pipe.
      char *fgets_rc = fgets (buffer, LINESIZE, pipe);
      if (fgets_rc == NULL) break;

      // Skip the line if it's directive.
      if (*buffer == '#') {
         continue;
      }

      // Scan of tokens using strtok_r()
      char *savepos = NULL;
      char *bufptr = buffer;
      for (;;) {
         char *token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = NULL;
         if (token == NULL) break;
         // Add the token to the stringset.
         intern_stringset (token);
      }
   }
}

int main (int argc, char** argv) {
   set_execname (argv[0]);

   while ( (option = getopt(argc, argv, OPT_STRING.c_str())) != -1 )  {
      switch(option) {
         case '?':
            fprintf(stderr, 
               "Usage: oc [-ly] [-@ flag] [-D string] program.oc\n");
            set_exitstatus(1);
            return get_exitstatus();
            break;
         case 'l':
            // enable this when flex is being used.
            // yy_flex_debug = 1;
            break;
         case 'y':
            // enable this when flex is being used.
            // yydebug = 1;
            break;
         case 'D':
            CPP += " -D" + string(optarg);
            break;
         case '@':
            set_debugflags(optarg);
            break;
         case 1:
            // Set the index of the program file
            if (file_arg_i) {
               // This is run if more than 1 file is passed.
               fprintf(stderr, 
                  "Usage: oc [-ly] [-@ flag] [-D string] program.oc\n");
               set_exitstatus(1);
               return get_exitstatus();
            }
            file_arg_i = optind-1;
            break;
      }
   }

   // Check that a file was passed to oc.
   if (!file_arg_i) {
      fprintf(stderr, 
         "Usage: oc [-ly] [-@ flag] [-D string] program.oc\n");
      set_exitstatus(1);
      return get_exitstatus();
   }
   char* filename = argv[file_arg_i];
   char* filebase = basename(filename);
   char* ext = strchr(filebase, '.');
   // If there is no period in the filename.
   if (ext == NULL) {
      fprintf(stderr, "Error: oc accepts .oc files only \n");
      set_exitstatus(1);
      return get_exitstatus();
   }
   char* program_name = strndup(filebase, ext-filebase);

   // Remove all trailing slashes
   for (int i = strlen(ext+1); i > 0; i--) {
      if (ext[i] == '/') {
         ext[i] = '\0';
      } else {
         break;
      }
   }

   // Check file extension
   if (strcmp(ext+1, "oc")) {
      fprintf(stderr, "Error: oc accepts .oc files only \n");
      set_exitstatus(1);
      return get_exitstatus();
   }

   // Open up the output file.
   string str_path = string(program_name) + ".str";
   FILE* str_file = fopen(str_path.c_str(), "w");

   // Open a pipe to cpp
   string command = CPP + " " + filename;
   FILE* pipe = popen (command.c_str(), "r");

   if (pipe == NULL) {
      syserrprintf (command.c_str());
      set_exitstatus(1);
   } else {
      cpplines (pipe);
      if (pclose (pipe) != 0) {
         // If cpp returns a non-zero code.
         set_exitstatus(1);
      }
   }

   dump_stringset (str_file);
   fclose(str_file);
   return get_exitstatus();
}
