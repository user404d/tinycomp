%{
#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "tinycomp.h"
#include "tinycomp.hpp"

/* Prototypes - for lex */
int yylex(void);
void yyerror(const char *s);

/* Global variables */
SymTbl *sym = new SimpleArraySymTbl();
TargetCode *code = new TargetCode();

const char* typestrs[] = {
	"integer",
	"floating point"
};

%}

%union{
	int iValue;					/* integer value */
	float fValue;				/* float value */
	char idLexeme;				/* identifiers */
	typeName typeLexeme;		/* lexemes for type id's */

	Attribute* attrs;			/* attributes for nonterminals */
	int inhAttr;    			/* inherited attribute storing address */
}

%token <idLexeme>ID <iValue>INTEGER <fValue>FLOAT <typeLexeme>TYPE
%token STAT

%token TRUE FALSE

%token WHILE IF PRINT
%nonassoc ELSE

%left OR AND

%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%type <attrs>expr
%type <attrs>stmt 
%type <attrs>stmt_list 
%type <attrs>cond

%%
prog:	decls stmt_list 		{ 
									TacInstr *i = code->gen(UNKNOWNOpr, NULL, NULL); 
									code->backpatch(((StmtAttr *)$2)->getNextlist(), i);
									code->printOut(); 
								}
	;

decls:	decls decl
	| decl
	;

decl: TYPE id_list ';'
	;

id_list:	id_list ',' ID 	{cout << "Recognizing var " << $3 << " of type " << typestrs[$<typeLexeme>0] << "\n"; }
	   | 	ID 				{cout << "Recognizing var " << $1 << " of type " << typestrs[$<typeLexeme>0] << "\n"; }
	;

stmt_list:
          stmt ';'          { $$ = $1; }
        | stmt_list 
          {$<inhAttr>$ = code->getNextInstr();} 
          stmt ';'       	{ 
				code->backpatch(((StmtAttr *)$1)->getNextlist(), code->getInstr($<inhAttr>2));

				$$ = $3;
			}
        ;

stmt:
	STAT 	{ 
				code->gen(fakeOpr, NULL, NULL);

				$$ = new StmtAttr();
			}
	| ID '=' expr	{
				code->gen(copyOpr, new VarAddress($1), ((ExprAttr*)$3)->getAddr());

				$$ = new StmtAttr();
			}
	| WHILE '(' 
	  {$<inhAttr>$ = code->getNextInstr();} 
	  cond ')' 
	  {$<inhAttr>$ = code->getNextInstr();}  
	  '{' stmt_list '}' {
				code->backpatch(((BoolAttr *)$4)->getTruelist(), code->getInstr($<inhAttr>6));

				TacInstr* i = code->gen(jmpOpr, code->getInstr($<inhAttr>3)->getValueNumber(), NULL);

				code->backpatch(((StmtAttr *)$8)->getNextlist(), i);

				StmtAttr *attrs = new StmtAttr();
				attrs->addNext(((BoolAttr *)$4)->getFalselist());

				$$ = attrs;
			}
	;

expr:
	INTEGER {
				Address *ia = new ConstAddress($1);
				// generate a copy instr where to store the integer
				TacInstr* i = code->gen(copyOpr, ia, NULL);

				$$ = new ExprAttr(i);
			}
	| ID 	{
				Address *ia = new ConstAddress($1);
			}
	;

cond:
	TRUE 	{
				BoolAttr* attrs = new BoolAttr();

				TacInstr* i = code->gen(jmpOpr, NULL, NULL);
				attrs->addTrue(i);

				$$ = attrs;
			}
	| FALSE {
				BoolAttr* attrs = new BoolAttr();

				TacInstr* i = code->gen(jmpOpr, NULL, NULL);
				attrs->addFalse(i);

				$$ = attrs;
			}
	| cond OR {$<inhAttr>$ = code->getNextInstr();} cond {
				code->backpatch(((BoolAttr *)$1)->getFalselist(), code->getInstr($<inhAttr>3));

				BoolAttr* attrs = new BoolAttr();
				attrs->addTrue(((BoolAttr *)$1)->getTruelist());
				attrs->addTrue(((BoolAttr *)$4)->getTruelist());

				attrs->addFalse(((BoolAttr *)$4)->getFalselist());

				$$ = attrs;
			} 
	;

%%

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}

int main(void) {
    yyparse();
}