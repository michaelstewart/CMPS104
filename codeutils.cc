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
vector<string> reg_vector;

char mangle_letter(string type) {
  if (type.compare("int") == 0) {
    return 'i';
  } else if (type.compare("ubyte") == 0) {
    return 'b';
  }
  return 'p';
}

string inttostr(int i) {
  ostringstream ss;
  ss << i;
  return ss.str();
}

string save_in_reg(string code, string type, size_t depth) {
  char buffer[10];
  sprintf (buffer, "%c%d", mangle_letter(type), ++counter);
  string s = indent + type + " " + buffer + " = " + code + ";\n";
  if (reg_vector.size() >= depth) {
    // Add to string
    reg_vector[depth-1] += s;
  } else {
    // Make new string
    reg_vector.push_back(s);
  }
  return buffer;
}

string get_regs() {
  string s = "";
  for (size_t i = reg_vector.size(); i > 0; i--) {
    // s += "[" + inttostr(i);
    s += reg_vector[i-1];
    // s += inttostr(i) + "]";
  }
  reg_vector.clear();
  return s;
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

string codegen(astree* root, bool save, int depth) {
  string code = "";
  if (save) depth++;

  switch (root->symbol) {
    case TOK_ROOT: {
      for(size_t i = 0; i < root->children.size(); i++) {
        code += codegen(root->children[i], false, depth) + ";\n";
      }
      break;
    }
    case DECL:
      // TODO: Check this
      code = codegen(root->children[0], false, depth);
      break;
    case TYPE:
      // code = *root->lexinfo;
      code = root->type;
      break;
    case BLOCK: {
      for(size_t i = 0; i < root->children.size(); i++) {
        code += codegen(root->children[i], false, depth) + ";\n";
      }
      break;
    }
    case IFELSE: {
      string num = inttostr(++counter);
      code += "if (!" + codegen(root->children[0], true, depth) + ") goto ";
      code += (root->children.size() > 2) ? "else" : "fi";
      code += "_" + num + ";\n";
      code += codegen(root->children[1], false, depth);
      if (root->children.size() > 2) {
        code += "\n";
        code += "else_" + num + ":;\n";
        code += codegen(root->children[2], false, depth);
      }
      code += "fi_" + num + ":";
      break;
    }
    case WHILE: {
      string num = inttostr(++counter);
      code += "\nwhile_" + num + ":;\n";
      code += indent + "if (!" + codegen(root->children[0], true, depth) + ") goto break_" + num + ";\n";
      code += codegen(root->children[1], false, depth);
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
          params += codegen(root->children[2]->children[i], false, depth);
        }
      }
      params += ")";
      int lastChld = root->children.size()-1;
      string block = codegen(root->children[lastChld], false, depth);
      code = "\n" + type + "\n" + name + params + "\n{\n" + block + "}\n";
      break;
    }
    case CALL: {
      code += mangle_name(*root->children[0]->lexinfo);
      code += "(";
      if (root->children.size() > 1) {
        for (size_t i = 0; i < root->children[1]->children.size(); i++) {
          if (i) code += ", ";
          code += codegen(root->children[1]->children[i], false, depth);
        }
      }
      code += ")";        
      break;
    }
    case BINOP: {
      string left = codegen(root->children[0], true, depth);
      string right = codegen(root->children[2], true, depth);
      string op = *root->children[1]->lexinfo;
      code = left + " " + op + " " + right;
      break;
    }
    case UNOP: {
      string op = *root->children[0]->lexinfo;
      string right = codegen(root->children[1], false, depth);
      code = op + right;
      break;
    }
    case VARIABLE: {
      code = codegen(root->children[0], true, depth);
      if (root->children.size() > 1) {
        if (root->children[1]->symbol == '[') {
          code += "[" + codegen(root->children[2], true, depth) + "]";
        } else if (root->children[1]->symbol == '.') {
          code += "[" + codegen(root->children[2], true, depth) + "]"; // FIX THIS
        }
      }
      break;
    }
    case CONSTANT: {
      code = codegen(root->children[0], false, depth);
      break;
    }
    case VARDECL: {
      string type = map_type(root->children[0]->type);
      string left = codegen(root->children[1], false, depth);
      string right = codegen(root->children[2], true, depth);
      code = indent + type + " " + left + " = " + right;

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
    code  = save_in_reg(code, map_type(root->type), depth);
  }
  if (depth == 0) {
    code = get_regs() + code;
  }

  return code;
}

void print_section_break(FILE* oil_file) {
  fprintf(oil_file, "\n");
}
void print_preamble(FILE* oil_file) {
  fprintf(oil_file, "#define __OCLIB_C__\n#include \"oclib.oh\"\n");
}

void print_structs(FILE* oil_file) {
  type_table->print_types(oil_file);
}

void print_globals(FILE* oil_file) {
  global_table->print_globals(oil_file);
}

void run_code_gen(astree* yyparse_astree, FILE* oil_file) {
  print_preamble(oil_file);
  print_section_break(oil_file);
  print_structs(oil_file);
  print_section_break(oil_file);
  print_globals(oil_file);
  print_section_break(oil_file);
  string out = codegen(yyparse_astree, false, 0);
  fprintf(oil_file, "%s", out.c_str());
}

