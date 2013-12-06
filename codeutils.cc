#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <iostream>
#include <sstream>
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
string indent = "        ";
stringstream ss;

char mangle_letter(string type) {
  if (type.compare("int") == 0) {
    return 'i';
  } else if (type.compare("ubyte") == 0) {
    return 'b';
  }
  return 'p';
}

string save_in_reg(string code, string type) {
  char buffer[10];
  sprintf (buffer, "%c%d", mangle_letter(type), ++counter);
  ss << type << " " << buffer << " = " << code << ";" << endl;
  return buffer;
}

string inttostr(int i) {
  ostringstream ss;
  ss << i;
  return ss.str();
}

string mangle_name(string name) {
  int level = global_table->lookupBlock(name);
  if (level == 0)
    return "__" + name;
  else
    return "_" + inttostr(level) + "_" + name;
}

string map_type(string type) {
  // cout << "this is a:" << type << endl;
  if (type.length() > 2 && type.substr(type.length()-2, 2).compare("[]") == 0) {
    return map_type(type.substr(0, type.length()-2)) + "*";
  } else {
    if (type.compare("bool") == 0 || type.compare("char") == 0) {
      return "ubyte";
    } else if (type.compare("int") == 0) {
      return "int";
    } else if (type.compare("string") == 0) {
      return "ubyte*";
    }
  }
  return type;
}

string codegen(astree* root, bool save) {
  string code = "";
  bool printSave = false;
  switch (root->symbol) {
    case TOK_ROOT: {
      for(size_t i = 0; i < root->children.size(); i++) {
        code += indent + codegen(root->children[i]) + ";\n";
      }
      break;
    }
    case DECL:
      // TODO: Check this
      code = codegen(root->children[0]);
      break;
    case TYPE:
      // code = *root->lexinfo;
      code = root->type;
      break;
    case BLOCK: {
      for(size_t i = 0; i < root->children.size(); i++) {
        code += indent + codegen(root->children[i]) + ";\n";
      }
      break;
    }
    case IFELSE: {
      string num = inttostr(++counter);
      code += "if (!" + codegen(root->children[0], true) + ") goto ";
      code += (root->children.size() > 2) ? "else" : "fi";
      code += "_" + num + ";\n";
      code += codegen(root->children[1]);
      if (root->children.size() > 2) {
        code += "\n";
        code += "else_" + num + ":;\n";
        code += codegen(root->children[2]);
      }
      code += "fi_" + num + ":";
      break;
    }
    case WHILE: {
      string num = inttostr(++counter);
      code += "\nwhile_" + num + ":;\n";
      code += indent + "if (!" + codegen(root->children[0], true) + ") goto break_" + num + ";\n";
      code += codegen(root->children[1]);
      code += indent + "goto while_" + num + ";\n";
      code += "break_" + num + ":";
      break;
    }
    case FUNCTION: {
      string type = map_type(root->children[0]->type);
      string name = mangle_name(*root->children[1]->lexinfo);
      string params = "(";
      if (root->children.size() > 3) {
        for (size_t i = 0; i < root->children[2]->children.size(); i++) {
          if (i) params += ", ";
          params += codegen(root->children[2]->children[i]);
        }
      }
      params += ")";
      int lastChld = root->children.size()-1;
      string block = codegen(root->children[lastChld], false);
      code = "\n" + type + "\n" + name + params + "\n{\n" + block + "}\n";
      break;
    }
    case CALL: {
      code += mangle_name(*root->children[0]->lexinfo);
      code += "(";
      if (root->children.size() > 1) {
        for (size_t i = 0; i < root->children[1]->children.size(); i++) {
          if (i) code += ", ";
          code += codegen(root->children[1]->children[i]);
        }
      }
      code += ")";        
      break;
    }
    case BINOP: {
      string left = codegen(root->children[0], true);
      string right = codegen(root->children[2], true);
      string op = *root->children[1]->lexinfo;
      code = left + " " + op + " " + right;
      printSave = true;
      break;
    }
    case UNOP: {
      string op = *root->children[0]->lexinfo;
      string right = codegen(root->children[1], false);
      code = op + right;
      break;
    }
    case VARIABLE: {
      code = codegen(root->children[0], false);
      if (root->children.size() > 1) {
        if (root->children[1]->symbol == '[') {
          code += "[" + codegen(root->children[2], false) + "]";
        } else if (root->children[1]->symbol == '.') {
          code += "[" + codegen(root->children[2], false) + "]"; // FIX THIS
        }
      }
      break;
    }
    case CONSTANT: {
      code = codegen(root->children[0]);
      break;
    }
    case VARDECL: {
      string type = map_type(root->children[0]->type);
      string left = codegen(root->children[1]);
      string right = codegen(root->children[2], true);
      printSave = true;
      code = type + " " + left + " = " + right;

      break;
    }
    case TOK_IDENT:
      code = mangle_name(*root->lexinfo);
      break;
    case TOK_INTCON:
      code = *root->lexinfo;
      break;
    case TOK_CHARCON:
      code = *root->lexinfo;
      break;
    case TOK_STRINGCON:
      code = *root->lexinfo;
      break;
    case TOK_FALSE:
      code = "0";
      break;
    case TOK_TRUE:
      code = "1";
      break;
    case TOK_NULL:
      code = "0";
      break;

  }
  // code = string(get_yytname(root->symbol)) + ":" + code;
  // cout << ss.str();  
  // code = ss.str() + code;
  // ss.str("");
  // code = "|" + code + "|";
  if (save) {
    code  = save_in_reg(code, map_type(root->type));
  } 
  if (printSave) {
    code = ss.str() + code;
    ss.str("");
  }
  return code;
}

string codegen(astree* root) {
  return codegen(root, false);
}

void print_section_break() {
  fprintf(stdout, "\n");
}
void print_preamble() {
  fprintf(stdout, "#define __OCLIB_C__\n#include \"oclib.oh\"\n");
}

void print_globals() {
  global_table->print_globals(stdout);
}

void run_code_gen(astree* yyparse_astree) {
  print_preamble();
  print_section_break();
  print_globals();
  print_section_break();
  cout << codegen(yyparse_astree);
}

