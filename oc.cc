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
const string OPT_STRING = "-lyD:@:";
int option;

int main (int argc, char** argv) {
   set_execname (argv[0]);

   while ( (option = getopt(argc, argv, OPT_STRING.c_str())) != -1 )  {
      switch(option) {
         case 'l':
            printf("%s\n", "l");
            break;
         case 'y':
            printf("%s\n", "y");
            break;
         case 'D':
            printf("%s\n", optarg);
            break;
         case '@':
            printf("%s\n", optarg);
            break;
      }
   }

   char* filename = argv[argc-1]; 
   char* ext;
   // char* p;
   
   ext = strchr(filename, '.');
   char* program_name = strndup(filename, ext-filename);
   // printf("extension - %s\nfilename - ",ext+1);
   if (!strcmp(ext+1, "oc")) {         
      printf("%s \n", program_name);
   } else {
      fprintf(stderr, "oc accepts .oc files only \n");
      return 1;
   }    

   string str_path = string(program_name);
   str_path = str_path + ".str";
   FILE* str_file = fopen(str_path.c_str(), "w");

   string command = CPP + " " + filename;
   FILE* pipe = popen (command.c_str(), "r");

   if (pipe == NULL) {
      syserrprintf (command.c_str());
   }else {
      cpplines (pipe, filename);
      int pclose_rc = pclose (pipe);
      eprint_status (command.c_str(), pclose_rc);
   }
      
   dump_stringset (str_file);
   fclose(str_file);

   return get_exitstatus();
}