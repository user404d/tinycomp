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

/* Mapping of types to their names */
const char* typestrs[] = {
	"integer",
	"floating point"
};

int TempAddress::counter = 0;

/* Global variables */
Memory& mem = Memory::getInstance();
SimpleArraySymTbl *sym = new SimpleArraySymTbl();
TargetCode *code = new TargetCode();

%}

/* This is the union that defines the type for var yylval,
 * which corresponds to 'lexval' in our textboox parlance.
 */
%union{
	/* tokens for constants */
	int iValue;					/* integer value */
	float fValue;				/* float value */

	/* tokens for other lexemes (var id's and generic lexemes) */
	char idLexeme;				/* identifiers */
	typeName typeLexeme;		/* lexemes for type id's */

	/* types for other syntactical elements: typically, attributes of the symbols */
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
									TacInstr *i = code->gen(haltOpr, NULL, NULL); 
									code->backpatch(((StmtAttr *)$2)->getNextlist(), i);

									/* ====== */
									cout << "*********" << endl;
									cout << "Size of int: " << sizeof(int) << endl;
									cout << "Size of float: " << sizeof(float) << endl;
									cout << "*********" << endl;
									cout << "== Symbol Table ==" << endl;
									sym->printOut();
									cout << endl;
									cout << "== Output (3-addr code) ==" << endl;
									code->printOut(); 
									/* ====== */
								}
	;

decls:	decls decl
	| decl
	;

decl: TYPE id_list ';'
	;

id_list:	id_list ',' ID 	{
								sym->put($3, $<typeLexeme>0);
								//cout << "Recognizing var " << $3 << " of type " << typestrs[$<typeLexeme>0] << "\n"; 
							}
	   | 	ID 				{
	   							sym->put($1, $<typeLexeme>0);
	   						}
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
				VarAddress* var = sym->get($1);
				code->gen(copyOpr, var, ((ExprAttr*)$3)->getAddr());

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
				ConstAddress *ia = new ConstAddress($1);

				$$ = new ExprAttr(ia);
			}
	| FLOAT {
				ConstAddress *ia = new ConstAddress($1);

				$$ = new ExprAttr(ia);
			}
	| ID 	{
				VarAddress *ia = sym->get($1);

				$$ = new ExprAttr(ia);
			}
	| expr '+' expr {
				// Note: I'm not handling all cases of type checking here; needs to be completed

				if ( ((ExprAttr*)$1)->getType() == intType && ((ExprAttr*)$3)->getType() == intType ) {
					TempAddress* temp = mem.getNewTemp(sizeof(int));

					TacInstr* i = code->gen(addOpr, ((ExprAttr*)$1)->getAddr(), ((ExprAttr*)$3)->getAddr(), temp);

					$$ = new ExprAttr(i, intType);
				} else {
				// else ... (all other type combinations should be considered here)
				// ...					
				}
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