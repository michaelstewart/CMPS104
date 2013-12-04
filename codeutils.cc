#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#include "codeutils.h"
#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include "symtable.h"
#include "typetable.h"

int counter = 0;
string save_in_reg(string code) {
  char buffer [10];
  sprintf (buffer, "t%d", ++counter);
  cout << " int " << buffer << " = " << code << ";" << endl;
  return buffer;
}

string codegen(astree* root, bool save) {
  string code = "";
  switch (root->symbol) {
    case TOK_ROOT: {
      for(size_t i = 0; i < root->children.size(); i++) {
        cout << codegen(root->children[i]) << ";" << endl;
      }
      break;
    }
    case BINOP: {
      // cout << "HERE1" << endl;
      string left = codegen(root->children [0], true);
      string right = codegen(root->children [2], true);
      code = left + " + " + right;
      break;
    }    
    case TOK_INTCON: {
      code = *root->lexinfo;
      break;
    }
    case VARDECL: {

      break;
    }
  }
  return save ? save_in_reg(code) : code;
}

string codegen(astree* root) {
  return codegen(root, false);
}

string map_type() {
  return "ubyte";
}

void print_preamble() {
  fprintf(stdout, "#define __OCLIB_C__\n#include \"oclib.oh\"\n");
}

void print_globals() {
  // global_table->print_globals(stdout);
}

void run_code_gen(astree* yyparse_astree) {
  print_preamble();
  print_globals();
  codegen(yyparse_astree);
}

