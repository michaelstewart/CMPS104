#ifndef __CODEUTILS_H__
#define __CODEUTILS_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "astree.h"


string save_in_reg(string code);
string codegen(astree* root, bool save, int depth);
string map_type(string type);
void run_code_gen(astree* yyparse_astree, FILE* oil_file);

extern int counter;

#endif
