#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "treeutils.h"
#include "astree.h"
#include "stringset.h"
#include "lyutils.h"
#include "symtable.h"

using namespace std;


/* Build TableTraversal */

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

/* Various Error Printing functions */
void raise_error(astree* one, astree* two, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s with %s\n", root->filenr, root->linenr, 
    root->offset, one->type.c_str(), two->type.c_str());
}

void raise_error(string type, astree* one, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s used with %s\n", root->filenr, root->linenr, 
    root->offset, type.c_str(), one->type.c_str());
}

void raise_error(string type, astree* root) {
  errprintf("Type error at (%d,%d,%d): %s\n", root->filenr, root->linenr, 
    root->offset, type.c_str());
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
        root->type = current_table->lookup(*root->children[0]->lexinfo);
      }
      break;
    }
    case TOK_RETURN: {
      string return_type = global_table->parseSignature(current_table->parentFunction(current_table))[0];
      root->type = return_type;
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

/* Type Checking Traversal */

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
      vector<string> args = global_table->parseSignature(root->type);
      if (args.size()-1 != root->children[1]->children.size()) {
        raise_error("Function called with incorrect number of args.", root->children[0]);
      }
      for(size_t i = 1; i < args.size(); i++) {
        if (args[i].compare(root->children[1]->children[i-1]->type) != 0) {
          raise_error("Function called with incorrect argument types.", root->children[0]);
        }
      }
      break;
    }
    case TOK_RETURN: {
      if (root->children.size() == 0) {
        if (root->type.compare("void") != 0)
          raise_error("Incorrect return type", root);
      } else {
        if (root->type.compare(root->children[0]->type) != 0)
          raise_error("Incorrect return type", root);
      }
      break;
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
