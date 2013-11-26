#include <string>
#include <vector>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "astree.h"
#include "treeutils.h"
#include "auxlib.h"
#include "lyutils.h"
#include "stringset.h"
#include "symtable.h"
#include "typetable.h"

string CPP = "/usr/bin/cpp";
string yyin_cpp_command;
const size_t LINESIZE = 1024;
const string OPT_STRING = "-lyD:@:";
int option;
int file_arg_i = 0;
extern int scan_linenr;
extern int scan_offset;
extern vector<string> included_filenames;
FILE* tok_file = NULL;

// Open a pipe from the C preprocessor.
// Exit failure if can't.
// Assignes opened pipe to FILE* yyin.
void yyin_cpp_popen (const char* filename) {
   yyin_cpp_command = CPP + " " + filename;
   yyin = popen (yyin_cpp_command.c_str(), "r");
   if (yyin == NULL) {
      syserrprintf (yyin_cpp_command.c_str());
      exit (get_exitstatus());
   }
}

void yyin_cpp_pclose (void) {
   int pclose_rc = pclose (yyin);
   if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

int main (int argc, char** argv) {
   set_execname (argv[0]);
   yy_flex_debug = 0;
   yydebug = 0;

   // Scan options
   while ( (option = getopt(argc, argv, OPT_STRING.c_str())) != -1 )  {
      switch(option) {
         case '?':
            fprintf(stderr, 
               "Usage: oc [-ly] [-@ flag] [-D string] program.oc\n");
            set_exitstatus(1);
            return get_exitstatus();
            break;
         case 'l':
            yy_flex_debug = 1;
            break;
         case 'y':
            yydebug = 1;
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

   // Open up the output files.
   string str_path = string(program_name) + ".str";
   FILE* str_file = fopen(str_path.c_str(), "w");
   str_path = string(program_name) + ".tok";
   tok_file = fopen(str_path.c_str(), "w");
   str_path = string(program_name) + ".ast";
   FILE* ast_file = fopen(str_path.c_str(), "w");

   yyin_cpp_popen(filename);

   yyparse();

   yyin_cpp_pclose();

   

   build_table_traversal(yyparse_astree);
   type_check_traversal(yyparse_astree);

   dump_astree(ast_file, yyparse_astree);

   printf("==================\n");
   global_table->dump(stdout, 0);
   
   DEBUGSTMT ('s', dump_stringset (stderr); );
   yylex_destroy();
   fclose(tok_file);

   // Dump the stringset
   dump_stringset (str_file);
   fclose(str_file);

   fclose(ast_file);

   return get_exitstatus();
}
