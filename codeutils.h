#ifndef __CODEUTILS_H__
#define __CODEUTILS_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "astree.h"


string save_in_reg(string code);
string codegen(astree* root, bool save);
string codegen(astree* root);
string map_type(string type);
void run_code_gen(astree* yyparse_astree);

extern int counter;

#endif
