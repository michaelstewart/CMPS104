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
   tree->noBlock = false;
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
   tree->noBlock = false;
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

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

bool print_symbol(int sym) {
  return (sym == TOK_ROOT || sym == PARAMETERS || sym == BLOCK || sym == VARDECL 
    || sym == WHILE || sym == IFELSE || sym == BINOP || sym == UNOP 
    || sym == ALLOCATOR || sym == CALL || sym == ARGS || sym == VARIABLE 
    || sym == CONSTANT || sym == STRUCTDEF || sym == MEMBERS || sym == DECL 
    || sym == TYPE || sym == BASETYPE || sym == FUNCTION);
}

static void dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
  if (root == NULL) return;
  if (print_symbol(root->symbol))
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

