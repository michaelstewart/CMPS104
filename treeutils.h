#ifndef __TREEUTILS_H__
#define __TREEUTILS_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "astree.h"

void build_table_traversal(astree* root);
void type_check_traversal(astree* root);

#endif
