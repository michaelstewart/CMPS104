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
   tree->symbol = symbol;
   tree->lexinfo = intern_stringset (lexinfo, true);
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

/* Traversal Functions */

string create

void preCase(astree* root) {
  switch(root->symbol) {
    case BLOCK: {
      table = table->enterBlock();
      break;
    }
    case VARDECL: {
      // printf("T: %s N: %s \n", (*root->children[1]->lexinfo).c_str(), (*root->children[0]->children[0]->children[0]->lexinfo).c_str());
      table->addSymbol(*root->children[1]->lexinfo, 
        *root->children[0]->children[0]->children[0]->lexinfo);
      break;
    }
    case DECL: {
      table->addSymbol(*root->children[1]->lexinfo, 
        *root->children[0]->children[0]->children[0]->lexinfo);
      break;
    }
    case VARIABLE: {
      if (root->children[0]->symbol == TOK_IDENT) {
        table->lookup(*root->children[0]->lexinfo);
      }
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

      table = table->enterFunction(*root->children[1]->lexinfo, return_type + parameters);
      break;
    }
    case CALL: {
      if (root->children[0]->symbol == TOK_IDENT) {
        // printf("%s\n", table->lookup(*root->children[0]->lexinfo).c_str());
        table->lookup(*root->children[0]->lexinfo);
      }
      break;
    }
  }
}

void postCase(astree* root) {
  switch(root->symbol) {
    case BLOCK:
      table = table->leaveBlock();
      break;
    case FUNCTION:
      table = table->leaveBlock();
      break;
  }
}

void preorderTraversal(astree* root) {
  // fprintf(stderr, "%s %d\n", root->lexinfo->c_str(), root->symbol);
  preCase(root);
  for(size_t i = 0; i < root->children.size(); i++) {
    preorderTraversal(root->children[i]);
  }
  postCase(root);

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

