#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include "symtable.h"

using namespace std;

astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo) {
   astree* tree = new astree();
   tree->type = string("");
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->lexinfo = intern_stringset (lexinfo, false);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

astree* new_astree (int symbol, const char* lexinfo) {
   astree* tree = new astree();
   tree->type = string("");
   tree->symbol = symbol;
   tree->lexinfo = intern_stringset (lexinfo, true);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

/* Traversal Functions */

string tok_const_type(int sym) {
  switch(sym) {
    case TOK_INTCON:
      return string("int");
    case TOK_CHARCON:
      return string("char");
    case TOK_STRINGCON:
      return string("string");
    case TOK_FALSE:
      return string("bool");
    case TOK_TRUE:
      return string("bool");
    case TOK_NULL:
      return string("null");
  }
  return string("");
}

void table_pre_case(astree* root) {
  switch(root->symbol) {
    case BLOCK: {
      // printf("%s\n", (*root->children[0]->lexinfo).c_str());
      current_table = current_table->enterBlock();
      break;
    }
    case VARDECL: {
      // printf("T: %s N: %s \n", (*root->children[1]->lexinfo).c_str(), (*root->children[0]->children[0]->children[0]->lexinfo).c_str());
      current_table->addSymbol(*root->children[1]->lexinfo, 
        *root->children[0]->children[0]->children[0]->lexinfo);
      break;
    }
    case DECL: {
      current_table->addSymbol(*root->children[1]->lexinfo, 
        *root->children[0]->children[0]->children[0]->lexinfo);
      break;
    }
    case VARIABLE: {
      if (root->children[0]->symbol == TOK_IDENT) {
        root->children[0]->type = current_table->lookup(*root->children[0]->lexinfo);
        root->type = root->children[0]->type;
      }
      break;
    }
    case CONSTANT: {
      root->children[0]->type = tok_const_type(root->children[0]->symbol);
      root->type = root->children[0]->type;
      break;
    }
    case TYPE: {
      break;
    }
    case FUNCTION: {
      string return_type = *root->children[0]->children[0]->children[0]->lexinfo;
      string parameters = "(";
      for(size_t i = 0; i < root->children[2]->children.size(); i++) {
        if (i) parameters += ',';
        if (root->children[2]->children[i]->symbol == DECL) {
          parameters += *root->children[2]->children[i]->children[0]->children[0]->children[0]->lexinfo;
        }        
      }
      parameters += string(")");

      current_table = current_table->enterFunction(*root->children[1]->lexinfo, return_type + parameters);
      break;
    }
    case CALL: {
      if (root->children[0]->symbol == TOK_IDENT) {
        // printf("%s\n", current_table->lookup(*root->children[0]->lexinfo).c_str());
        current_table->lookup(*root->children[0]->lexinfo);
      }
      break;
    }
  }
}

void table_post_case(astree* root) {
  switch (root->symbol) {
    case FUNCTION:
      current_table = current_table->leaveBlock();
      break;
    case BLOCK:
      current_table = current_table->leaveBlock();
      break;
  }
}

void build_table_traversal(astree* root) {
  // fprintf(stderr, "%s %d\n", root->lexinfo->c_str(), root->symbol);
  table_pre_case(root);
  for(size_t i = 0; i < root->children.size(); i++) {
    build_table_traversal(root->children[i]);
  }
  table_post_case(root);

}


// void type_pre_case(astree* root) {
//   switch(root->symbol) {
//     case VARIABLE: {
//       if (root->children[0]->symbol == TOK_IDENT) {
//         root->children[0]->type = lookup(*root->children[0]->lexinfo);
//         root->type = root->children[0]->type;
//       }
//       break;
//     }
//   }
// }


string check_prim(string type) {
  if (type.compare("int") == 0 || type.compare("bool") == 0 
    || type.compare("char") == 0  || type.compare("string") == 0 
    || type.compare("null") == 0) {
    return string("primitive");
  }
  return type;
}

string check_base(string type) {
  return string("basetype");
}

bool check_types(string type, string one) {
  if (type.compare("primitive") == 0) {
    one = check_prim(one);
  } else if (type.compare("basetype") == 0) {
    one = check_base(one);
  }
  return (type.compare(one) == 0);
}

bool check_types(string type, string one, string two) {
  if (type.compare("primitive") == 0) {
    one = check_prim(one);
    two = check_prim(two);
  } else if (type.compare("basetype") == 0) {
    one = check_base(one);
    two = check_base(two);
  }
  return (type.compare(one) == 0 && type.compare(two) == 0);
}

void raise_error(astree* one, astree* two, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s with %s\n", root->filenr, root->linenr, 
    root->offset, one->type.c_str(), two->type.c_str());
}

void raise_error(string type, astree* one, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s used with %s\n", root->filenr, root->linenr, 
    root->offset, type.c_str(), one->type.c_str());
}

void type_post_case(astree* root) {
  switch(root->symbol) {
    case BINOP: {
      int sym = root->children[1]->symbol;
      if (sym == '+' || sym == '-' || sym == '*' || sym == '/' || sym == '%') {
        if (!check_types("int", root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
        } else {
          root->type = string("int");
        }
      } else if (sym == TOK_LT || sym == TOK_LE || sym == TOK_GT || sym == TOK_GE) {
        if (!check_types("primitive", root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
        } else {
          root->type = string("bool");
        }
      } else if (sym == '=') {
        // anytype
        root->type = string("anytype");
      } else if (sym == TOK_EQ || sym == TOK_NE) {
        // anytype
        root->type = string("bool");
      }
    }
    case UNOP: {
      int sym = root->children[0]->symbol;
      if (sym == '!') {
        if (!check_types("bool", root->children[1]->type)) {
          raise_error("!", root->children[1], root->children[0]);
        } else {
          root->type = string("bool");
        }
      } else if (sym == '+' || sym == '-') {
        if (!check_types("int", root->children[1]->type)) {
          raise_error("+ or -", root->children[1], root->children[0]);
        } else {
          root->type = string("int");
        }
      } else if (sym == TOK_ORD) {
        if (!check_types("char", root->children[1]->type)) {
          raise_error("ord", root->children[1], root->children[0]);
          root->type = string("int");
        } else {
          root->type = string("int");
        }
      } else if (sym == TOK_CHR) {
        if (!check_types("int", root->children[1]->type)) {
          raise_error("chr", root->children[1], root->children[0]);
        } else {
          root->type = string("char");
        }
      }
      break;
    }
    case IFELSE: {
      if (!check_types("bool", root->children[0]->type)) {
        raise_error("if", root->children[0], root->children[0]);
      }
      break;
    }
    case WHILE: {
      if (!check_types("bool", root->children[0]->type)) {
        raise_error("while", root->children[0], root->children[0]);
      }
      break;
    }
    case ALLOCATOR: {
      if (!check_types("basetype", root->children[0]->type)) {
        raise_error("allocator", root->children[0], root->children[0]);
      }
      break;
    }
    case CALL: {
      if (!check_types("primitive", root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
      } else {
          root->type = string("bool");
      }
    }
  }
}

void type_check_traversal(astree* root) {
  // fprintf(stderr, "%s %d\n", root->lexinfo->c_str(), root->symbol);
  // type_pre_case(root);
  for(size_t i = 0; i < root->children.size(); i++) {
    type_check_traversal(root->children[i]);
  }
  type_post_case(root);
}

/* Traversal Functions */


astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt_front (astree* root, astree* child) {
   root->children.insert(root->children.begin(), child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}



static void dump_node (FILE* outfile, astree* node) {
   fprintf(outfile, "%4lu%4lu.%.03lu %4d  %-15s (%s)\n", 
     node->filenr, node->linenr, node->offset,
     node->symbol, get_yytname (node->symbol), 
     node->lexinfo->c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
      fprintf (outfile, "%p", node->children.at(child));
   }
}

static void dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
  if (root == NULL) return;
  if (root->symbol == TOK_ROOT)
    fprintf (outfile, "%*s%s\n", depth * 3, "",
    root->lexinfo->c_str());
  else
    fprintf (outfile, "%*s%s (%s)\n", depth * 3, "",
    get_yytname (root->symbol), root->lexinfo->c_str());
   for (size_t child = 0; child < root->children.size();
        ++child) {
      dump_astree_rec (outfile, root->children[child],
                       depth + 1);
   }
}

void dump_astree (FILE* outfile, astree* root) {
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}

void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      // fprintf (outfile, "%s(%d)\n",
               // get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
           root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}

