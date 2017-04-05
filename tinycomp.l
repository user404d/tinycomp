%{
#include <stdlib.h>
#include "tinycomp.h"
#include "tinycomp.tab.h"

void yyerror(const char *);
%}

%option noyywrap

/* regular definitions */
intconst        0|[1-9][0-9]*
floatconst      {intconst}\.[0-9]*

%%

"int"       {
                yylval.typeLexeme = intType;
                return TYPE;

            }

"float"     {
                yylval.typeLexeme = floatType;
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

[-()<>=+*/,;{}.] {
                return *yytext;
             }

"//".*          { /* Skip 1-line comments */ }

[ \t\n]+        ;       /* ignore whitespace */

.               {
                    const char* err = "Unknown character";
                    yyerror(err);
                }

%%
