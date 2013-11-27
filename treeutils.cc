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
#include "typetable.h"

using namespace std;

TypeTable* in_structdef = NULL;

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

string tok_base_type(int sym) {
  switch(sym) {
    case TOK_VOID:
      return string("null");
    case TOK_INT:
      return string("int");
    case TOK_CHAR:
      return string("char");
    case TOK_STRING:
      return string("string");
    case TOK_BOOL:
      return string("bool");
  }
  return string("");
}

/* Various Error Printing functions */
void raise_error(astree* one, astree* two, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s with %s\n", root->filenr, root->linenr, 
    root->offset, one->type.c_str(), two->type.c_str());
}

void raise_error(astree* one, astree* two, astree* root, string details) {
  errprintf("Type mismatch at (%d,%d,%d): %s: %s with %s\n", root->filenr, root->linenr, 
    root->offset, details.c_str(), one->type.c_str(), two->type.c_str());
}

void raise_error(string type, astree* one, astree* root) {
  errprintf("Type mismatch at (%d,%d,%d): %s used with %s\n", root->filenr, root->linenr, 
    root->offset, type.c_str(), one->type.c_str());
}

void raise_error(string type, astree* root) {
  errprintf("Type error at (%d,%d,%d): %s\n", root->filenr, root->linenr, 
    root->offset, type.c_str());
}

string stripBrackets(string str) {
    vector<string> sig = global_table->parseSignature(str);
    if (sig.size() > 0) {
      return sig[0];
    }
    return string("");
}

void table_pre_case(astree* root) {
  switch(root->symbol) {
    case BLOCK: {
      // printf("%s\n", (*root->children[0]->lexinfo).c_str());
      if (root->noBlock)
        break;
      current_table = current_table->enterBlock();
      break;
    }
    case VARDECL: {
      // printf("VARDECL - N: %s T: %s \n", (*root->children[1]->lexinfo).c_str(), (root->children[0]->type).c_str());
      string type = *root->children[0]->children[0]->children[0]->lexinfo;
      if (root->children[0]->children.size() > 1) {
        // If it's an array add [] to end
        type += "[]";
      } 
      current_table->addSymbol(*root->children[1]->lexinfo, type);
      current_table->addLine(*root->children[1]->lexinfo, root->children[1]->filenr, root->children[1]->linenr, root->children[1]->offset);
      break;
    }
    case DECL: {
      if (in_structdef != NULL)
        break;
      // printf("DECL - T: %s N: %s \n", (*root->children[1]->lexinfo).c_str(), (*root->children[0]->children[0]->children[0]->lexinfo).c_str());  
       string type = *root->children[0]->children[0]->children[0]->lexinfo;
      if (root->children[0]->children.size() > 1) {
        // If it's an array add [] to end
        type += "[]";
      }      

      current_table->addSymbol(*root->children[1]->lexinfo, type);
      current_table->addLine(*root->children[1]->lexinfo, root->children[1]->filenr, root->children[1]->linenr, root->children[1]->offset);
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
    case FUNCTION: {
      string return_type = *root->children[0]->children[0]->children[0]->lexinfo;
      if (root->children[0]->children.size() > 1) {
        // If it's an array add [] to end
        return_type += "[]";
      } 

      string parameters = "";
      if (root->children.size() == 4) {
        // If there are params
        parameters += "(";
        for(size_t i = 0; i < root->children[2]->children.size(); i++) {
          if (i) parameters += ',';
          astree* t = root->children[2]->children[i];
          if (root->children[2]->children[i]->symbol == DECL) {
            parameters += *t->children[0]->children[0]->children[0]->lexinfo;
            if (t->children[0]->children.size() > 1) {
              // If it's an array add [] to end
              parameters += "[]";
            }
          }
        }
        parameters += ")";
      } else if(root->children.size() == 3) {
        // No params
        parameters = "()";
      }

      root->children[root->children.size()-1]->noBlock = true;

      current_table->addLine(*root->children[1]->lexinfo, root->children[1]->filenr, root->children[1]->linenr, root->children[1]->offset);
      current_table = current_table->enterFunction(*root->children[1]->lexinfo, return_type + parameters);
      break;
    }
    case CALL: {
      if (root->children[0]->symbol == TOK_IDENT) {
        // printf("%s\n", current_table->lookup(*root->children[0]->lexinfo).c_str());
        // root->type = stripBrackets(current_table->lookup(*root->children[0]->lexinfo));
        root->type = current_table->lookup(*root->children[0]->lexinfo);
      }
      break;
    }
    case TOK_RETURN: {
      root->type = stripBrackets(current_table->parentFunction(current_table));
      break;
    }
    case TOK_STRUCT: {
      in_structdef = type_table->addStruct(*root->children[0]->lexinfo);
      break;
    }
  }
}

void table_post_case(astree* root) {
  switch (root->symbol) {
    case TOK_STRUCT: {
      for(size_t i = 0; i < root->children[1]->children.size(); i++) {
        in_structdef->addType(*root->children[1]->children[i]->children[1]->lexinfo, root->children[1]->children[i]->children[0]->type);
        root->children[1]->children[i]->children[1]->type = root->children[1]->children[i]->children[0]->type;
      }
      in_structdef = NULL;
      break;
    }
    case FUNCTION: {
      current_table = current_table->leaveBlock();
      break;
    }
    case BLOCK: {
      if (root->noBlock)
        break;
      current_table = current_table->leaveBlock();
      break;
    }
    case BASETYPE: {
      if (root->children[0]->symbol == TOK_IDENT) {
        if (type_table->lookupType(*root->children[0]->lexinfo) != NULL) {
          root->children[0]->type = *root->children[0]->lexinfo;
          root->type = root->children[0]->type;
        }
      } else {
        root->type = tok_base_type(root->children[0]->symbol);
      }
      break;
    }
    case TYPE: {
      root->type = root->children[0]->type;
      if (root->children.size() > 1) {
        // If it's an array add [] to end
        root->type += "[]";
      } 
      break;
    }
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

/* Type Checking Helpers */

bool check_prim(string type) {
  if (type.compare("int") == 0 || type.compare("bool") == 0 
    || type.compare("char") == 0  || type.compare("string") == 0 
    || type.compare("null") == 0) {
    return true;
  }
  return false;
}

bool check_prim(string type1, string type2) {
  return check_prim(type1) && check_prim(type2);
}

bool check_base(string type) {
  if (check_prim(type)) {
    return true;
  } else if (type_table->lookupType(type) != NULL) {
    return true;
  }
  return false;
}

bool check_base(string type1, string type2) {
  return check_base(type1) && check_base(type2);
}

bool eq(string type1, string type2) {
  if (type1.compare("null") == 0 || type2.compare("null") == 0)
    return true;
  return (type1.compare(type2) == 0);
}

bool check_types(string type, string one, string two) {
  // cout << "T:" << type << " one:" << one << " two:" << two;
  if (type.compare("primitive") == 0) {
    if (!check_prim(one, two)) {
      return false;
    } else {
      return (one.compare(two) == 0);
    }
  } else if (type.compare("basetype") == 0) {
    if (!check_base(one, two)) {
      return false;
    } else {
      return (one.compare(two) == 0);
    }
  } else if (type.compare("") == 0) {
    return eq(one, two);
  }
  return (type.compare(one) == 0) && (type.compare(two) == 0);  
}

bool check_types(string one, string two) {
  return check_types("", one, two);
}

bool check_type(string type, string one) {
  if (type.compare("primitive") == 0) {
    return check_prim(one);
  } else if (type.compare("basetype") == 0) {
    return check_base(one);
  } else {
    return (type.compare(one) == 0);
  }
}

/* End Type Checking Helpers */

void type_post_case(astree* root) {
  
  switch(root->symbol) {
    case BINOP: {
      int sym = root->children[1]->symbol;
      if (sym == '+' || sym == '-' || sym == '*' || sym == '/' || sym == '%') {
        if (!check_types("int", root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
        }
        root->type = string("int");
      } else if (sym == TOK_LT || sym == TOK_LE || sym == TOK_GT || sym == TOK_GE) {        
        if (!check_types("primitive", root->children[0]->type, root->children[2]->type)) {        
          raise_error(root->children[0], root->children[2], root->children[1]);
        } 
        root->type = string("bool");
      } else if (sym == '=') {
        // anytype
        if (!check_types(root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
        } 
        root->type = string("anytype");
      } else if (sym == TOK_EQ || sym == TOK_NE) {
        if (!check_types(root->children[0]->type, root->children[2]->type)) {
          raise_error(root->children[0], root->children[2], root->children[1]);
        } 
        root->type = string("bool");
      }
    }
    case UNOP: {
      int sym = root->children[0]->symbol;
      if (sym == '!') {
        if (!check_type("bool", root->children[1]->type)) {
          raise_error("!", root->children[1], root->children[0]);
        } 
        root->type = string("bool");
      } else if (sym == '+' || sym == '-') {
        if (!check_type("int", root->children[1]->type)) {
          raise_error("+ or -", root->children[1], root->children[0]);
        } 
        root->type = string("int");
      } else if (sym == TOK_ORD) {
        if (!check_type("char", root->children[1]->type)) {
          raise_error("ord", root->children[1], root->children[0]);
        } 
        root->type = string("int");
      } else if (sym == TOK_CHR) {
        if (!check_type("int", root->children[1]->type)) {
          raise_error("chr", root->children[1], root->children[0]);
        } 
        root->type = string("char");
      }
      break;
    }
    case IFELSE: {
      if (!check_type("bool", root->children[0]->type)) {
        raise_error("if", root->children[0], root->children[0]);
      }
      break;
    }
    case WHILE: {
      if (!check_type("bool", root->children[0]->type)) {
        raise_error("while", root->children[0], root->children[0]);
      }
      break;
    }
    case ALLOCATOR: {
      if (root->children[0]->children[0]->symbol == TOK_IDENT) {
        // We're looking for a struct
        // cout << "STRUCT: " << *root->children[0]->children[0]->lexinfo << endl;
        if (type_table->lookupType(*root->children[0]->children[0]->lexinfo) != NULL) {
          root->type = root->children[0]->type;
        }
      } else if (root->children[1]->symbol == '[') {
       root->type = root->children[0]->type + "[]";
      } else if (root->children[1]->symbol == '(') {
       root->type =root->children[0]->type;
      } else {
        if (!check_type("basetype", root->children[0]->type)) {
          raise_error("allocator", root->children[0], root->children[0]);
        }
      }
      break;
    }
    case VARDECL: {
      if (!check_type(root->children[0]->type, root->children[2]->type)) {
        raise_error(root->children[0], root->children[2], root->children[1], "improper variable declaration");
      }
      break;
    }
    case VARIABLE: {
      if (root->children.size() == 3) {
        // index into string -> char
        if (root->children[1]->symbol == '[') {
          // Either index into string or index into array
          if (check_type("string", root->children[0]->type)) {
            // We have an index into a string
            if (!check_type("int", root->children[2]->type)) {
              raise_error("Incorrect index into string", root->children[1]);
            } else {
              root->type = string("char");
            } 
          } else {
            // We have an index into an array
            // Strip off the last two chars (should be [])
            string typeBase = root->children[0]->type.substr(0, root->children[0]->type.length()-2);
            if (!check_base(typeBase) || !check_type("int", root->children[2]->type)) {
              raise_error("Incorrect index into array", root->children[1]);
            } else {
              root->type = typeBase;
            }
          }                   
        }
        // Access of a structs element
       if (root->children[1]->symbol == '.') {
          TypeTable* local = type_table->lookupType(root->children[0]->type);
          // lookup that type
          if (local == NULL) {
            raise_error("Type does not exist", root->children[1]);
          } else {
            root->type = local->lookup(*root->children[2]->lexinfo);
            // lookup the type of the element within the struct that was found
            if (root->type.compare("") == 0) {
              raise_error("Invalid struct element", root->children[1]);
            }
          }        
        } 
      }
      break;
    }
    case CALL: {
      vector<string> args = global_table->parseSignature(root->type);
      size_t numArgs;
      if (root->children.size() < 2) {
        numArgs = 0;
      } else {
        numArgs = root->children[1]->children.size();
      }
      if (args.size()-1 != numArgs) {
        raise_error("Function called with incorrect number of arguments.", root->children[0]);
      }
      
      for(size_t i = 1; i < args.size(); i++) {
        if (args[i].compare(root->children[1]->children[i-1]->type) != 0) {
          raise_error("Function called with incorrect argument types.", root->children[0]);
        }
      }
      root->type = stripBrackets(root->type);
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
