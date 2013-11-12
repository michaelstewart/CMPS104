%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "astree.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYPRINT yyprint
#define TTMALLOC yycalloc

astree* new_parsenode (const char* type_str);

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_ROOT

%destructor { error_destructor ($$); } <>

%right  '='
%left   TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left   '+' '-'
%left   '*' '/' '%'
%right  POS "u+" NEG "u-" '!' TOK_ORD TOK_CHR
%left   '(' ')'
%left   '[' ']' '.'
%nonassoc TOK_NEW

%start  program

%%

program   : program structdef                                 { $$ = adopt1 ($1, $2);  }
          | program function                                  { $$ = adopt1 ($1, $2);  }
          | program statement                                 { $$ = adopt1 ($1, $2);  }
          |                                                   { $$ = new_parseroot(); }
          ;

structdef : TOK_STRUCT TOK_IDENT '{' slist '}'                { free_ast2($3, $5); $$ = adopt2 ($1, $2, $4); }
          ;
slist     : slist decl ';'                                    { free_ast($3); $$ = adopt1 ($1, $2); }
          |
          ;
decl      : type TOK_IDENT                                    { $$ = adopt1 ($1, $2); }
          ;
type      : basetype                                          { $$ = adopt1(new_parsenode("type"), $1); }
          | basetype TOK_NEWARRAY                             { $$ = adopt2(new_parsenode("type"), $1, $2); }
          ;          
basetype  : TOK_VOID                                          { $$ = adopt1(new_parsenode("basetype"), $1); }
          | TOK_BOOL                                          { $$ = adopt1(new_parsenode("basetype"), $1); }
          | TOK_CHAR                                          { $$ = adopt1(new_parsenode("basetype"), $1); }
          | TOK_INT                                           { $$ = adopt1(new_parsenode("basetype"), $1); }
          | TOK_STRING                                        { $$ = adopt1(new_parsenode("basetype"), $1); }
          | TOK_IDENT                                         { $$ = adopt1(new_parsenode("basetype"), $1); }
          ;
function  : type TOK_IDENT '(' ')' block                      { free_ast2($3, $4); $$ = adopt2(new_parsenode("function"), $1, $2); adopt1($$, $5)}
          | type TOK_IDENT '(' parameters ')' block           { free_ast2($3, $5); $$ = adopt2(new_parsenode("function"), $1, $2); adopt2($$, $4, $6)}
          ;
          ;
parameters: parameters ',' decl                               { free_ast($2); $$ = adopt1 ($1, $3); }
          | decl                                              { $$ = adopt1(new_parsenode("parameters"), $1); }
          ;
block     : '{' blist '}'                                     { free_ast2($1, $3); $$ = $2; }
          | ';'                                               { free_ast($1); $$ = new_parsenode("block"); }
          ;
blist     : blist statement                                   { $$ = adopt1($1, $2); }
          |                                                   { $$ = new_parsenode("block"); }                                             
          ;
statement : block                                             { $$ = $1; }
          | vardecl                                           { $$ = $1; }
          | while                                             { $$ = $1; }
          | ifelse                                            { $$ = $1; }
          | return                                            { $$ = $1; }
          | expr ';'                                          { $$ = $1; }
          ;
vardecl   : type TOK_IDENT '=' expr ';'                       { free_ast2 ($3, $5); $$ = adopt2(new_parsenode("vardecl"),$1, $2); adopt1($$, $4); }  
          ;
while     : TOK_WHILE '(' expr ')' statement                  { free_ast($1); free_ast2($2, $4); $$ = adopt2(new_parsenode("while"), $3, $5); }
          ;          
ifelse    : TOK_IF '(' expr ')' statement TOK_ELSE statement  { free_ast2 ($2, $4); $$ = adopt2 ($1, $3, $5); }
          | TOK_IF '(' expr ')' statement                     { free_ast2 ($2, $4); $$ = adopt2 ($1, $3, $5); }

          ;
return    : TOK_RETURN ';'                                    { free_ast2 ($1, $2); $$ = new_parsenode("return"); }
          | TOK_RETURN expr ';'                               { free_ast2 ($1, $3); $$ = adopt1(new_parsenode("return"), $2); }
          ;
expr      : allocator                                         { $$ = $1; }
          | call                                              { $$ = $1; }
          | unop                                              { $$ = $1; }
          | binop                                             { $$ = $1; }
          | '(' expr ')'                                      { free_ast2 ($1, $3); $$ = $2; }
          | variable                                          { $$ = $1; }
          | constant                                          { $$ = $1; }
          ;
binop     : expr '=' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_EQ expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_NE expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_LT expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_LE expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_GT expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr TOK_GE expr                                  { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr '+' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr '-' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr '*' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr '/' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          | expr '%' expr                                     { $$ = new_parsenode("binop"); adopt2($$, $1, $2); adopt1($$, $3); }
          ;
unop      : '+' expr %prec POS                                { $$ = adopt2(new_parsenode("unop"), $1, $2); /*$$ = adopt1sym ($1, $2, POS);*/ }
          | '-' expr %prec NEG                                { $$ = adopt2(new_parsenode("unop"), $1, $2); }
          | '!' expr                                          { $$ = adopt2(new_parsenode("unop"), $1, $2); }
          | TOK_ORD expr                                      { $$ = adopt2(new_parsenode("unop"), $1, $2); }
          | TOK_CHR expr                                      { $$ = adopt2(new_parsenode("unop"), $1, $2); }
          ;
allocator : TOK_NEW basetype '(' expr ')'                     { free_ast2($3, $5); $$ = adopt2(new_parsenode("allocator"), $2, $3); }
          | TOK_NEW basetype '[' expr ']'                     { free_ast2($3, $5); $$ = adopt2(new_parsenode("allocator"), $2, $3); }
          ;
call      : TOK_IDENT '(' ')'                                 { $$ = adopt1(new_parsenode("call"), $1); }
          ;
call      : TOK_IDENT '(' clist ')'                           { $$ = adopt1($3, $1); }
          ;
clist     : expr                                              { $$ = adopt1(new_parsenode("call"), $1); }
          | expr ',' clist                                    { $$ = adopt1($1, $3); }
          ;
variable  : TOK_IDENT                                         { $$ = adopt1(new_parsenode("variable"), $1); }
          | expr '[' expr ']'   
          | expr '.' TOK_IDENT  
          ;
constant  : TOK_INTCON                                        { $$ = adopt1(new_parsenode("constant"), $1); }
          | TOK_CHARCON                                       { $$ = adopt1(new_parsenode("constant"), $1); }
          | TOK_STRINGCON                                     { $$ = adopt1(new_parsenode("constant"), $1); }
          | TOK_FALSE                                         { $$ = adopt1(new_parsenode("constant"), $1); }
          | TOK_TRUE                                          { $$ = adopt1(new_parsenode("constant"), $1); }
          | TOK_NULL                                          { $$ = adopt1(new_parsenode("constant"), $1); }
          ;


%%


const char *get_yytname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*
static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != NULL);
   return result;
}
*/

