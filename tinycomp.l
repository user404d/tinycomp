%{
#include <stdlib.h>
#include "tinycomp.h"
#include "tinycomp.tab.h"

void yyerror(const char *);
%}

%option noyywrap
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

0           {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[1-9][0-9]* {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[-()<>=+*/,;{}.] {
                return *yytext;
             }


[ \t\n]+        ;       /* ignore whitespace */

.               {
                    const char* err = "Unknown character";
                    yyerror(err);
                }

%%
