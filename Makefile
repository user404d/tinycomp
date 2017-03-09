LEX_FILES = $(wildcard *.l)
BISON_FILES = $(wildcard *.y)
TAB_FILES = $(BISON_FILES:%.y=%.tab.c)
TAB_H_FILES = $(BISON_FILES:%.y=%.tab.h)
OBJ_FILES = $(TAB_FILES:%.tab.c=%.tab.o) lex.yy.o tinycomp.o

CC = g++

.PHONY: all lexcheck bisoncheck

all: lexcheck bisoncheck compiler

# Could 'exit 1' or the like if you don't want students to have multiples of these files
lexcheck: $(LEX_FILES)
	@if [[ $(words $(LEX_FILES)) -ne 1 ]]; then echo 'WARNING: More than one .l file; using $<'; fi

bisoncheck: $(BISON_FILES)
	@if [[ $(words $(BISON_FILES)) -ne 1 ]]; then echo 'WARNING: More than one .y file; using $<'; fi

lex.yy.c: $(firstword $(LEX_FILES))
	flex $<

%.tab.c: %.y lex.yy.c
	bison -d $<

library: $(OBJ_FILES)

compiler: library
	g++ $(OBJ_FILES) -o tinycomp

clean:
	rm lex.yy.c $(TAB_FILES) $(TAB_H_FILES) *.o tinycomp