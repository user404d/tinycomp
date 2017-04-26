%{
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tinycomp.h"
#include "tinycomp.hpp"

  using namespace std;
  /* Prototypes - for lex */
  int yylex(void);
  void yyerror(const char *s);

  void printout();

  /* Mapping of types to their names */
  const char* typestrs[] = {
    "integer",
    "floating point",
    "fraction"
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
%union
 {
   /* tokens for constants */
   int iValue;                             /* integer value */
   float fValue;                           /* float value */
   Fraction fracValue;

   /* tokens for other lexemes (var id's and generic lexemes) */
   char idLexeme;                          /* identifiers */
   typeName typeLexeme;                    /* lexemes for type id's */

   /* types for other syntactical elements: typically, attributes of the symbols */
   Attribute* attrs;                       /* attributes for nonterminals */
   int inhAttr;                            /* inherited attribute storing address */
}

%token <idLexeme>ID <iValue>INTEGER <fValue>FLOAT <fracValue>FRACTION <typeLexeme>TYPE
%token STAT

%token TRUE FALSE

%token WHILE IF THEN PRINT
%nonassoc ELSE

%left OR AND

%left GE LE REQ NE SEQ '>' '<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS ASSIGN

%type <attrs>expr
%type <attrs>stmt
%type <attrs>stmt_list
%type <attrs>cond

%%
prog: 
decls stmt_list 
{
  // add the final 'halt' instruction
  TacInstr *i = code->gen(haltOpr, nullptr, nullptr);
  code->backpatch(((StmtAttr *)$2)->getNextlist(), i);

  // print out the output IR, as well as some other info
  // useful for debugging
  printout();
}
;

decls: 
decls decl
| decl
;

decl: 
TYPE id_list ';'
;

id_list: 
id_list ',' ID
{
  sym->put($3, $<typeLexeme>0);
}
| ID
{
  sym->put($1, $<typeLexeme>0);
}
;

stmt_list:
stmt ';'          
{ 
  $$ = $1; 
}
| stmt_list
{
  $<inhAttr>$ = code->getNextInstr();
}
stmt ';'
{
  code->backpatch(((StmtAttr *)$1)->getNextlist(), code->getInstr($<inhAttr>2));

  $$ = $3;
}
;

stmt:
STAT
{
  code->gen(fakeOpr, nullptr, nullptr);

  $$ = new StmtAttr();
}
| ID ASSIGN expr        // ID := EXPR
{
  VarAddress* var = sym->get($1);

  if(var == nullptr) {
    yyerror("Uninitialized variable");
    assert(false);
  }

  const int width = Type::size.at(var->getType());
  ExprAttr* ex = static_cast<ExprAttr*>($3);
  // See tinycomp.h for typeTree promotion conditions
  // Compute promotion type
  const typeName promo = static_cast<typeName>(var->getType() ^ ex->getType());

  // Determine which instructions are necessary based on the types
  if(promo == typeTree::IDENTITY || promo == typeTree::FLOATPROMO) {
    if(var->getType() != typeTree::fracType) {
      code->gen(copyOpr, var, ex->getAddr());
    }
    else {
      /** Use two temporaries to copy fraction expression numerator
          and denominator into the variable numerator and denominator.
          Two constants are necessary for variable offsets.
          num = 0
          denom = sizeof(Fraction)/2
          u = ex[num]
          v = ex[denom]
          var[num] = u
          var[denom] = v
       */
      TempAddress * u = mem.getNewTemp(width/2),
        * v = mem.getNewTemp(width/2);
      ConstAddress * num = new ConstAddress(0),
        * denom = new ConstAddress(width/2);
      code->gen(offsetOpr, ex->getAddr(), num, u);
      code->gen(offsetOpr, ex->getAddr(), denom, v);
      code->gen(indexCopyOpr, num, u, var);
      code->gen(indexCopyOpr, denom, v, var);
    }
  }
  else if(var->getType() == typeTree::fracType && ex->getType() == typeTree::intType) {
    /** Promote the integer expression to a fraction and copy
        the value into the variable using appropriate offsets.
        Two constants are necessary for variable offsets.
        num = 0
        denom = sizeof(Fraction)/2
        var[num] = ex
        var[denom] = 1
    */
    ConstAddress * num = new ConstAddress(0),
      * denom = new ConstAddress(width/2);
    code->gen(indexCopyOpr, num, ex->getAddr(), var);
    code->gen(indexCopyOpr, denom, new ConstAddress(1), var);
  }
  else {
    yyerror("Type mismatch");
    assert(false);
  }

  $$ = new StmtAttr();
}
| WHILE '('          // while(
{
  $<inhAttr>$ = code->getNextInstr();
}
cond ')'             // COND)
{
  $<inhAttr>$ = code->getNextInstr();
}
'{' stmt_list '}'    // { BODY }
{
  code->backpatch(((BoolAttr *)$4)->getTruelist(), code->getInstr($<inhAttr>6));

  TacInstr* i = code->gen(jmpOpr, nullptr, nullptr, code->getInstr($<inhAttr>3)->getValueNumber());

  code->backpatch(((StmtAttr *)$8)->getNextlist(), i);

  StmtAttr *attrs = new StmtAttr();
  attrs->addNext(((BoolAttr *)$4)->getFalselist());

  $$ = attrs;
}
| IF '(' cond ')'      // if ( COND )
{
  $<inhAttr>$ = code->getNextInstr();
}
THEN '{' stmt_list '}'
{
  /** Essentially the while loop without the jump back to check the
      condition added to the end of the stmt_list body.
   */
  code->backpatch(((BoolAttr *)$3)->getTruelist(), code->getInstr($<inhAttr>5));

  StmtAttr *attrs = new StmtAttr();
  attrs->addNext(((BoolAttr *)$3)->getFalselist());

  $$ = attrs;
}
;

expr:
INTEGER 
{
  ConstAddress *ia = new ConstAddress($1);

  $$ = new ExprAttr(ia);
}
| FLOAT
{
  ConstAddress *ia = new ConstAddress($1);

  $$ = new ExprAttr(ia);
}
| FRACTION
{
  /** Fraction constant needs to be loaded into a temporary of the
      appropriate width. Two constants are necessary for appropriate
      offsets and two constants are necessary for the fraction
      numerator and denominator.
      num = 0
      denom = sizeof(Fraction)/2
      temp[num] = $1.num
      temp[denom] = $1.denom
   */
  const int width = Type::size.at(typeTree::fracType);
  TempAddress * temp = mem.getNewTemp(width);
  ConstAddress * num = new ConstAddress(0),
    * denom = new ConstAddress(width/2);

  code->gen(indexCopyOpr, num, new ConstAddress($1.num), temp);
  code->gen(indexCopyOpr, denom, new ConstAddress($1.denom), temp);

  $$ = new ExprAttr(temp, typeTree::fracType);
}
| ID
{
  VarAddress *ia = sym->get($1);

  $$ = new ExprAttr(ia);
}
| expr '|' expr
{
  /** Fraction expression which needs to be placed in a temporary of
      the appropriate width. Two constants are necessary for offsets
      into the temporary variable.
      num = 0
      denom = sizeof(Fraction) / 2
      temp[num] = ex1
      temp[denom] = ex2
   */
  const int width = Type::size.at(typeTree::fracType);
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  TempAddress * temp = nullptr;
  ConstAddress * num = new ConstAddress(0),
    * denom = new ConstAddress(width/2);
  
  if(ex1->getType() != typeTree::intType || ex2->getType() != typeTree::intType) {
    yyerror("Non-integer used within fraction expression");
    assert(false);
  }

  temp = mem.getNewTemp(width);
  code->gen(indexCopyOpr, num, ex1->getAddr(), temp);
  code->gen(indexCopyOpr, denom, ex2->getAddr(), temp);

  $$ = new ExprAttr(temp, typeTree::fracType);
}
| expr '+' expr
{
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  TempAddress * temp = nullptr;
  TacInstr * i = nullptr;

  switch(ex1->getType() ^ ex2->getType()) {
  case typeTree::IDENTITY:
    {
      temp = mem.getNewTemp(Type::size.at(ex1->getType()));
      i = code->gen(addOpr, ex1->getAddr(), ex2->getAddr(), temp);
      $$ = new ExprAttr(i, ex1->getType());
      break;
    }
  case typeTree::FRACPROMO: /* TBD */
  case typeTree::FLOATPROMO:
  default:
    {
      yyerror("Type mismatch");
      assert(false);
    }
  }
}
| expr '/' expr
{
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  TempAddress * temp = nullptr;
  TacInstr * i = nullptr;

  switch(ex1->getType() ^ ex2->getType())
    {
    case typeTree::IDENTITY:
      {
        temp = mem.getNewTemp(Type::size.at(ex1->getType()));
        i = code->gen(divOpr, ex1->getAddr(), ex2->getAddr(), temp);
        $$ = new ExprAttr(i, ex1->getType());
        break;
      }
    case typeTree::FRACPROMO: /* TBD */
    case typeTree::FLOATPROMO:
    default:
      {
        yyerror("Type mismatch");
        assert(false);
      }
    }
}
| expr '*' expr
{
  // multiplication
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  TempAddress * temp = nullptr;

  /** Compute multiplication operation and store result in a
      temporary of appropriate width.
   */
  switch(ex1->getType() ^ ex2->getType()) {
  case typeTree::IDENTITY:
    {
      if(ex1->getType() != typeTree::fracType) {
        temp = mem.getNewTemp(Type::size.at(ex1->getType()));
        code->gen(mulOpr, ex1->getAddr(), ex2->getAddr(), temp);
      }
      else {
        /** Two temporaries are necessary for computing the
            result of the fraction * fraction operation. Two
            constants are necessary for offsets.
            num = 0
            denom = sizeof(Fraction) / 2
            t = ex1[num]
            u = ex2[num]
            t = t * u
            temp[num] = t
            (similarly for denominator)
         */
        const int width = Type::size.at(typeTree::fracType),
          offset = width/2;
        temp = mem.getNewTemp(width);
        TempAddress * t = mem.getNewTemp(offset),
          * u = mem.getNewTemp(offset);
        ConstAddress * num = new ConstAddress(0),
          * denom = new ConstAddress(offset);

        code->gen(offsetOpr, ex1->getAddr(), num, t);
        code->gen(offsetOpr, ex2->getAddr(), num, u);
        code->gen(mulOpr, t, u, t);
        code->gen(indexCopyOpr, num, t, temp);
        code->gen(offsetOpr, ex1->getAddr(), denom, t);
        code->gen(offsetOpr, ex2->getAddr(), denom, u);
        code->gen(mulOpr, t, u, t);
        code->gen(indexCopyOpr, denom, t, temp);
      }
      $$ = new ExprAttr(temp, ex1->getType());
      break;
    }
  case typeTree::FRACPROMO:
    {
      /** Simply multiply numerator of the fraction expression
          with integer and store denominator directly.
       */
      const int width = Type::size.at(typeTree::fracType),
        offset = width/2;
      temp = mem.getNewTemp(width);
      TempAddress * t = mem.getNewTemp(offset),
        * u = mem.getNewTemp(offset);
      ConstAddress * num = new ConstAddress(0),
        * denom = new ConstAddress(offset);

      code->gen(offsetOpr, ex1->getAddr(), num, t);
      code->gen(offsetOpr, ex2->getAddr(), num, u);
      code->gen(mulOpr, t, u, t);
      code->gen(indexCopyOpr, num, t, temp);

      if(ex1->getType() == typeTree::fracType) {
        code->gen(offsetOpr, ex1->getAddr(), denom, t);
      }
      else {
        code->gen(offsetOpr, ex2->getAddr(), denom, t);
      }
      code->gen(indexCopyOpr, denom, t, temp);
      $$ = new ExprAttr(temp, typeTree::fracType);
      break;
    }
  case typeTree::FLOATPROMO:
    {
      /** Simply do the multiplication and promote to float.
       */
      temp = mem.getNewTemp(Type::size.at(ex1->getType()));
      code->gen(mulOpr, ex1->getAddr(), ex2->getAddr(), temp);
      $$ = new ExprAttr(temp, typeTree::floatType);
      break;
    }
  default:
    {
      yyerror("Type mismatch");
      assert(false);
    }
  }
}
;

cond:
TRUE
{
  BoolAttr* attrs = new BoolAttr();

  TacInstr* i = code->gen(jmpOpr, nullptr, nullptr);
  attrs->addTrue(i);

  $$ = attrs;
}
| FALSE
{
  BoolAttr* attrs = new BoolAttr();

  TacInstr* i = code->gen(jmpOpr, nullptr, nullptr);
  attrs->addFalse(i);

  $$ = attrs;
}
| expr SEQ expr
{
  // boolean strict equality
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  BoolAttr * attrs = new BoolAttr();
  TacInstr * t = nullptr,
    * f = nullptr;

  switch(ex1->getType() ^ ex2->getType()) {
  case typeTree::IDENTITY:
    {
      if(ex1->getType() != typeTree::fracType) {
        /** Generate two jumps, the first is for when the comparison
            is true and the second is for when the comparison is false.
         */
        t = code->gen(jeOpr, ex1->getAddr(), ex2->getAddr(), nullptr);
        f = code->gen(jmpOpr, nullptr, nullptr);
        attrs->addTrue(t);
        attrs->addFalse(f);
      }
      else {
        /** Generate four jumps: the first compares the numerators,
            the second for when the first fails, the third compares
            the denominators, and the fourth is for when the third
            fails. The first needs to be patched to point to the
            address of the beginning of the block preparing for the
            third jump.
        */
        const int offset = Type::size.at(ex1->getType()) / 2;
        TempAddress * u = mem.getNewTemp(offset),
          * v = mem.getNewTemp(offset);
        ConstAddress * num = new ConstAddress(0),
          * denom = new ConstAddress(offset);
        TacInstr * tt = nullptr,
          * ff = nullptr;

        code->gen(offsetOpr, ex1->getAddr(), num, u);
        code->gen(offsetOpr, ex2->getAddr(), num, v);
        t = code->gen(jeOpr, u, v, nullptr);
        f = code->gen(jmpOpr, nullptr, nullptr);
        attrs->addFalse(f);
        tt = code->gen(offsetOpr, ex1->getAddr(), denom, u);
        t->patch(tt);
        code->gen(offsetOpr, ex2->getAddr(), denom, v);
        tt = code->gen(jeOpr, u, v, nullptr);
        ff = code->gen(jmpOpr, nullptr, nullptr);
        attrs->addTrue(tt);
        attrs->addFalse(ff);
      }
      break;
    }
  default:
    {
      yyerror("Type Mismatch");
      assert(false);
    }
  }
  $$ = attrs;
}
| expr REQ expr
{
  // boolean lax equality
  ExprAttr * ex1 = static_cast<ExprAttr*>($1),
    * ex2 = static_cast<ExprAttr*>($3);
  BoolAttr * attrs = new BoolAttr();
  TacInstr * t = nullptr,
    * f = nullptr;

  switch(ex1->getType() ^ ex2->getType()) {
  case typeTree::IDENTITY:
    {
      if(ex1->getType() != typeTree::fracType) {
        /** Generate two jumps, the first is for when the comparison
            is true and the second is for when the comparison is false.
         */
        t = code->gen(jeOpr, ex1->getAddr(), ex2->getAddr(), nullptr);
        f = code->gen(jmpOpr, nullptr, nullptr);
        attrs->addTrue(t);
        attrs->addFalse(f);
      }
      else {
        /** Generate two jumps: compute the floating point value
            of ex1[num]/ex1[denom] and ex2[num]/ex2[denom] then
            compare those values.
         */
        const int offset = Type::size.at(typeTree::fracType) / 2;
        TempAddress * u = mem.getNewTemp(offset),
          * v = mem.getNewTemp(offset),
          * w = mem.getNewTemp(offset);
        ConstAddress * num = new ConstAddress(0),
          * denom = new ConstAddress(offset);

        code->gen(offsetOpr, ex1->getAddr(), num, u);
        code->gen(offsetOpr, ex1->getAddr(), denom, v);
        code->gen(divOpr, u, v, u);
        code->gen(offsetOpr, ex2->getAddr(), num, v);
        code->gen(offsetOpr, ex2->getAddr(), denom, w);
        code->gen(divOpr, v, w, v);
        t = code->gen(jeOpr, u, v, nullptr);
        f = code->gen(jmpOpr, nullptr, nullptr);
        attrs->addTrue(t);
        attrs->addFalse(f);
      }
      break;
    }
  case typeTree::FRACPROMO:
    {
      /** Generate two jumps: depending on which expression is a
          fraction, compute the result of ex[num]/ex[denom] then
          compare the result to the integer expression.
       */
      const int offset = Type::size.at(typeTree::fracType) / 2;
      TempAddress * u = mem.getNewTemp(offset),
        * v = mem.getNewTemp(offset);
      ConstAddress * num = new ConstAddress(0),
        * denom = new ConstAddress(offset);

      if(ex1->getType() == typeTree::fracType) {
        code->gen(offsetOpr, ex1->getAddr(), num, u);
        code->gen(offsetOpr, ex1->getAddr(), denom, v);
        code->gen(divOpr, u, v, u);
        t = code->gen(jeOpr, u, ex2->getAddr(), nullptr);
      }
      else {
        code->gen(offsetOpr, ex2->getAddr(), num, u);
        code->gen(offsetOpr, ex2->getAddr(), denom, v);
        code->gen(divOpr, u, v, u);
        t = code->gen(jeOpr, u, ex1->getAddr(), nullptr);
      }

      f = code->gen(jmpOpr, nullptr, nullptr);
      attrs->addTrue(t);
      attrs->addFalse(f);
      break;
    }
  case typeTree::FLOATPROMO: /* TBD */
  default:
    {
      yyerror("Type Mismatch");
      assert(false);
    }
  }
  $$ = attrs;
}
| cond OR
{
  $<inhAttr>$ = code->getNextInstr();
} 
cond
{
  code->backpatch(((BoolAttr *)$1)->getFalselist(), code->getInstr($<inhAttr>3));

  BoolAttr* attrs = new BoolAttr();
  attrs->addTrue(((BoolAttr *)$1)->getTruelist());
  attrs->addTrue(((BoolAttr *)$4)->getTruelist());

  attrs->addFalse(((BoolAttr *)$4)->getFalselist());

  $$ = attrs;
}
;

%%
void printout() {
  /* ====== */
  cout << "*********" << endl;
  cout << "Size of int: " << sizeof(int) << endl;
  cout << "Size of float: " << sizeof(float) << endl;
  cout << "Size of Fraction: " << sizeof(Fraction) << endl;
  cout << "*********" << endl;
  cout << endl;
  cout << "== Symbol Table ==" << endl;
  sym->printOut();
  cout << endl;
  cout << "== Memory Dump ==" << endl;
  // mem.hexdump();
  mem.printOut(sym);
  cout << endl;
  cout << endl;
  cout << "== Output (3-addr code) ==" << endl;
  code->printOut();
  /* ====== */
}


void yyerror(const char *s) {
  cerr << s << endl;
}

int main(void) {
  return yyparse();
}
