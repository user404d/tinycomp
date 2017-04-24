%{
#include <string>
#include <stdlib.h>
#include "tinycomp.h"
#include "tinycomp.tab.h"

void yyerror(const char *);
%}

%option noyywrap

/* regular definitions */
natural         [1-9][0-9]*
intconst        0|{natural}
floatconst      {intconst}\.[0-9]*
fracconst       {intconst}\|{natural}

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
"=="            return REQ;
"!="            return NE;
"="             return SEQ;
":="            return ASSIGN;

"||"            return OR;
"&&"            return AND;

"while"         return WHILE;
"if"            return IF;
"then"          return THEN;
"else"          return ELSE;
"print"         return STAT; // TODO: Modify to print value

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

{fracconst} {
                // get numerator and denominator
                std::string fraction(yytext);
                std::string::size_type pos = 0;
                pos = fraction.find("|");
                yylval.fracValue = Fraction(std::stoi(fraction.substr(0,pos)),
                                            std::stoi(fraction.substr(pos+1)));
                return FRACTION;
             }

[-()<>=+*/,;{}.|] {
                return *yytext;
             }

"//".*          { /* Skip 1-line comments */ }

[ \t\n]+        ;       /* ignore whitespace */

.               {
                    const char* err = "Unknown character";
                    yyerror(err);
                }

%%
