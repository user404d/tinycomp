%{
#include <iostream>
#include <algorithm>
#include <string>
#include <stdlib.h>
#include "tinycomp.h"
#include "tinycomp.tab.h"

void yyerror(const char *);
%}

%option noyywrap

/* regular definitions */
intconst        0|[1-9][0-9]*
floatconst      {intconst}\.[0-9]*
fracconst       {intconst}\|{intconst}

%%

"int"       {
                yylval.typeLexeme = intType;
                return TYPE;

            }

"float"     {
                yylval.typeLexeme = floatType;
                return TYPE;
            }

"fraction"  {
                yylval.typeLexeme = fracType;
                return TYPE;
            }

"stat"      {
                return STAT;
            }


">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;

"||"            return OR;
"&&"            return AND;


"while"         return WHILE;
"if"            return IF;
"else"          return ELSE;
"print"         return PRINT;

"true"          return TRUE;
"false"         return FALSE;

[a-z]       {
                yylval.idLexeme = yytext[0];
                yyerror("ID");
                return ID;
            }

{intconst}  {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

{floatconst} {
                yylval.fValue = atof(yytext);
                return FLOAT;
             }

{fracconst} {
                // get numerator and denominator
                yyerror("Fraction");
                std::string fraction(yytext);
                std::string::size_type pos = 0;
                pos = fraction.find("|");
                yylval.fracValue.num = std::stoi(fraction.substr(0,pos));
                yylval.fracValue.denom = std::stoi(fraction.substr(pos));
                return FRACTION;
             }

[-()<>=+*/,:;{}.|] {
                return *yytext;
             }

"//".*          { /* Skip 1-line comments */ }

[ \t\n]+        ;       /* ignore whitespace */

.               {
                    const char* err = "Unknown character";
                    yyerror(err);
                }

%%
