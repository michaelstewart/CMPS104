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
string functions = "";
bool inAssignment = false;
map<int, string> reg_map;

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

string save_in_reg(string code, string type, int depth) {
  char buffer[10];
  if (type.compare("void") == 0)
    type = "int";
  sprintf (buffer, "%c%d", mangle_letter(type), ++counter);
  string s = indent + type + " " + buffer + " = " + code + ";\n";
  if (reg_map.count(depth) == 0) {
    reg_map[depth] = "";
  }
  reg_map[depth] += s;
  return buffer;
}

string get_regs() {
  string s = "";
  for( map<int, string>::reverse_iterator it=reg_map.rbegin(); it!=reg_map.rend(); ++it ) {
    // s += "[" + inttostr(it->first);
    s += it->second;
    // s += inttostr(it->first) + "]";
  }
  reg_map.clear();
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
  if (type.length() > 2 && type.substr(type.length()-2, 2).compare("[]") == 0) {
    return map_type(type.substr(0, type.length()-2)) + "*";
  } else {
    if (type.compare("bool") == 0 || type.compare("char") == 0) {
      return "ubyte";
    } else if (type.compare("int") == 0) {
      return "int";
    } else if (type.compare("string") == 0) {
      return "ubyte*";
    } else if (type.compare("null") == 0) {
      return "void";
    } else if (type.compare("void") == 0) {
      return "void";
    }
  }
  cout << "HERE:" << "struct " + type + " *" << endl;
  return "struct " + type + " *";
}

string clearRegs(string code, int depth) {
  if (depth == 0) {
    return get_regs() + code;
  }
  return code;
}

string makeBlock(astree* child, int depth) {
  string returnStr = codegen(child, false, depth);
  if (child->symbol != BLOCK) {
    return returnStr + ";\n";
  }
  return returnStr;
}

string codegen(astree* root, bool save, int depth) {
  string code = "";
  if (save) depth++;

  switch (root->symbol) {
    case TOK_ROOT: {
      for(size_t i = 0; i < root->children.size(); i++) {
        string s = codegen(root->children[i], false, depth);
        if (!s.length())
          continue;
        code += s + ";\n";
      }
      break;
    }
    case DECL:
      // TODO: Check this
      code = map_type(codegen(root->children[0], false, depth));
      if (root->children.size() > 1) {
        code += " " + codegen(root->children[1], false, depth);
      }

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
      code += indent + "if (!" + codegen(root->children[0], true, depth) + ") goto else_" + num + ";\n";
      code = clearRegs(code, depth);
      code += makeBlock(root->children[1], depth);
      code += indent + "goto fi_" + num + ";\n";
      code += "else_" + num + ":;\n";
      if (root->children.size() > 2) {        
        code = clearRegs(code, depth);
        code += makeBlock(root->children[2], depth);
      }
      code += "fi_" + num + ":";
      break;
    }
    case WHILE: {
      string num = inttostr(++counter);
      code += indent + "if (!" + codegen(root->children[0], true, depth) + ") goto break_" + num + ";\n";
      code = clearRegs(code, depth);      
      code += makeBlock(root->children[1], depth);
      code += indent + "goto while_" + num + ";\n";
      code += "break_" + num + ":";
      code = "\nwhile_" + num + ":;\n" + code;
      break;
    }
    case FUNCTION: {
      if (root->children[root->children.size()-1]->symbol != BLOCK)
        break;
      string type = map_type(root->children[0]->type);
      string name = mangle_name(*root->children[1]->lexinfo);
      string params = "(";
      if (root->children.size() > 3) {
        for (size_t i = 0; i < root->children[2]->children.size(); i++) {
          if (i) params += ",";
          params += "\n" + indent;
          params += codegen(root->children[2]->children[i], false, depth);
        }
      }
      params += ")";
      int lastChld = root->children.size()-1;
      string block = codegen(root->children[lastChld], false, depth);
      functions += "\n" + type + "\n" + name + params + "\n{\n" + block + "}\n";
      break;
    }
    case CALL: {
      code += mangle_name(*root->children[0]->lexinfo);
      code += "(";
      if (root->children.size() > 1) {
        for (size_t i = 0; i < root->children[1]->children.size(); i++) {
          if (i) code += ", ";
          code += codegen(root->children[1]->children[i], false, depth+1);
        }
      }
      code += ")";
      break;
    }
    case BINOP: {
      string op = *root->children[1]->lexinfo;
      inAssignment = (op.compare("=") == 0);
      string left = codegen(root->children[0], !inAssignment, depth);
      string right = codegen(root->children[2], true, depth);
      code = left + " " + op + " " + right;
      break;
    }
    case UNOP: {
      string op = *root->children[0]->lexinfo;
      if (op.compare("ord") == 0) {
        op = "(int) ";
      }
      string right = codegen(root->children[1], false, depth);
      code = op + right;
      break;
    }
    case ALLOCATOR: {
      string size = "1";
      if (root->children.size() > 1) {
        size = codegen(root->children[1]->children[0], false, depth);
      }
      code += "xcalloc(" + size + ", sizeof(" + map_type(root->type) + "))";
      break;
    }
    case VARIABLE: {
      code = codegen(root->children[0], !inAssignment, depth);
      if (root->children.size() > 1) {
        if (root->children[1]->symbol == '[') {
          code += "[" + codegen(root->children[2], true, depth) + "]";
        } else if (root->children[1]->symbol == '.') {
          code += "." + codegen(root->children[2], true, depth);
        }
      }
      inAssignment = false;
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
      if (left.substr(0,2).compare("__") == 0) {
        code = indent + left + " = " + right;
      } else {
        code = indent + type + " " + left + " = " + right;
      }      
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

  if (save) {
    code  = save_in_reg(code, map_type(root->type), depth);
  }
  code = clearRegs(code, depth);

  return code;
}

void print_section_break(FILE* oil_file) {
  fprintf(oil_file, "\n");
}
void print_preamble(FILE* oil_file) {
  fprintf(oil_file, "#define __OCLIB_C__\n#include \"oclib.oh\"\n");
  print_section_break(oil_file);
}

void print_structs(FILE* oil_file) {
  type_table->print_types(oil_file);
  print_section_break(oil_file);
}

void print_globals(FILE* oil_file) {
  global_table->print_globals(oil_file);
  print_section_break(oil_file);
}

void print_functions(FILE* oil_file) {
  fprintf(oil_file, "%s", functions.c_str());
  print_section_break(oil_file);
}

void print_main(FILE* oil_file) {
  fprintf(oil_file, "void __ocmain() {\n");
}

void print_main_end(FILE* oil_file) {
  fprintf(oil_file, "}\n");
}


void run_code_gen(astree* yyparse_astree, FILE* oil_file) {
  // Generate Code
  string out = codegen(yyparse_astree, false, 0);

  print_preamble(oil_file);
  print_structs(oil_file);
  print_globals(oil_file);
  print_functions(oil_file);
  print_main(oil_file);
  fprintf(oil_file, "%s", out.c_str());
  print_main_end(oil_file);
}

