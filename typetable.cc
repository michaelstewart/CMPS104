#include "auxlib.h"
#include "typetable.h"

TypeTable *type_table = new TypeTable(NULL);

// Creates and returns a new symbol table.
//
// Use "new TypeTable(NULL)" to create the global table
TypeTable::TypeTable(TypeTable* parent) {
  // Set the parent (this might be NULL)
  this->parent = parent;
}

// Add a symbol with the provided name and type to the current table.
//
// Example: To add the variable declaration "int i = 23;"
//          use "currentTypeTable->addSymbol("i", "int");
void TypeTable::addType(string name, string type) {
  // Use the variable name as key for the identifier mapping
  this->types[name] = type;
}

// Creates a new empty table beneath the current table and returns it.
TypeTable* TypeTable::addStruct(string name) {
  // Create a new symbol table beneath the current one
  TypeTable* child = new TypeTable(this);
  this->children[name] = child;
  return child;
}

TypeTable* TypeTable::lookupType(string name) {
  // Look up "name" in the identifier mapping of the current block
  if (this->children.count(name) > 0) {
    // If we found an entry, just return its type
    return this->children[name];
  }
  return NULL;
}

// Look up name in this and all surrounding blocks and return its type.
//
// Returns the empty string "" if variable was not found
string TypeTable::lookup(string id) {
  if (this->types.count(id) > 0) {
    // If we found an entry, just return its type
    return this->types[id];
  }
  return string("");
}

