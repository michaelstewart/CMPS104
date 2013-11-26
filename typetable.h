#ifndef __TYPEABLE_H__
#define __TYPEABLE_H__

#include <stdio.h>

#include <string>
#include <vector>
#include <map>

using namespace std;

// A symbol table for a single scope, i.e. block.
// It might reference its surrounding and inner scopes
// (the parent and children symbol tables).
class TypeTable {

  // Pointer to the parent symbol table
  // (might be NULL for the global table)
  TypeTable* parent;

  // The mapping of identifiers to their types
  map<string, string> types;
  map<string, TypeTable*> children;

public:
  // Creates and returns a new TypeTable table.
  TypeTable(TypeTable* parent);

  // Add a symbol with the provided name and type to the current table.
  //
  // Example: To add the variable declaration "int i = 23;"
  //          use "currentTypeTable->addSymbol("i", "int");
  void addType(string name, string type);

  TypeTable* addStruct(string name);

  TypeTable* lookupType(string name);

  // Look up name in this and all surrounding blocks and return its type.
  //
  // Returns the empty string "" if variable was not found
  string lookup(string id);

};

extern TypeTable *type_table;

#endif
