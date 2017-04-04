#include <iostream>
#include <iomanip>
#include <list>

#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

using namespace std;

#include "tinycomp.hpp"

const char* opTable[] = {
	"UNKNOWN",
	"HALT",
	"=",
	"+",
	"*",
	"goto",
	"ifgoto",
	"stat"
};

/**************************/
/* REPRESENTING ADDRESSES */
/**************************/

/*
 * ConstAddress
 */
ConstAddress::ConstAddress(int i) {
	type = intType;
	val.i = i;
}

ConstAddress::ConstAddress(float f) {
	type = floatType;
	val.f = f;
}

/** Returns the constant's type (as a typeName enum)
 */
typeName ConstAddress::getType() {
	return type;
}

const char* ConstAddress::toString() const {
	char* str = (char*)malloc(10*sizeof(char));

	switch(type) {
		case intType: snprintf(str, 10, "%d", val.i);
			break;
		case floatType: snprintf(str, 10, "%2.2f", val.f);
			break;
		default: strncpy(str, "?", 1);
			break;
	}

	return str;
}

/** Constructor: creates a variable address from its id (assuming only 1-char id's).
 */
VarAddress::VarAddress(char v, typeName t, int o) {
	lexeme= v;

	type = t;

	switch(type) {
		case intType:
			width = sizeof(int);
			break;
		case floatType:
			width = sizeof(float);
			break;
		default:
			/* should never reach here */
			break;
	}

	offset = o;
}

/** Returns the variable's type (as a typeName enum)
 */
typeName VarAddress::getType() {
	return type;
}

/** Returns the variable's width, which depends on its type
 */
int VarAddress::getWidth() {
	return width;
}

/** Returns the pointer to the memory location holding the variable's value
 */
int VarAddress::getOffset() {
	return offset;
}

const char* VarAddress::toString() const {
	char* str = (char*)malloc(2*sizeof(char));
	str[0] = lexeme;
	str[1] = '\0';

	return str;
}


/** Constructor: creates a temporary at the specified offset in memory
	 */
TempAddress::TempAddress(int offset) {
    // update the counter after using the old value as the "name" of this temporary
	name = counter++;

	this->offset = offset;
}

/** Returns the pointer to the memory location holding the temporary
 */
int TempAddress::getOffset() {
	return offset;
}

/** Concrete method for printing a TempAddress;
 *  it's a concrete implementation of the corresponding abstract method in Address
 */
const char* TempAddress::toString() const {
	char* str = (char*)malloc(5*sizeof(char));;
	sprintf(str, "t%d", name);
	
	return str;
}


/* 
 * InstrAddress 
 */
InstrAddress::InstrAddress(int vn) {
	arrayCodeIndex = vn;
}

const char* InstrAddress::toString() const {
	char* str = (char*)malloc(10*sizeof(char));

	snprintf(str, 10, "(%d)", arrayCodeIndex);

	return str;
}

/****************************/
/* COMPILER DATA STRUCTURES */
/****************************/

/* TargetCode 
 */

TacInstr* TargetCode::gen(TacInstr* instr) {
	instr->setValueNumber(nextInstr);
	codeArray[nextInstr] = instr;

	nextInstr++;

	return instr;
}

TacInstr* TargetCode::gen(oprEnum op, Address* operand1, Address* operand2) {
	return gen(new TacInstr(op, operand1, operand2, NULL));
}

TacInstr* TargetCode::gen(oprEnum op, Address* operand1, Address* operand2, TempAddress* temp) {
	return gen(new TacInstr(op, operand1, operand2, temp));
}

TargetCode::TargetCode() {
	nextInstr = 0;
}

TacInstr* TargetCode::getInstr(int i) {
	return codeArray[i];
} 

int TargetCode::getNextInstr() {
	return nextInstr;
}

void TargetCode::backpatch(list<TacInstr*> l, TacInstr* i) {
	list<TacInstr*>::iterator it;
	 for (it = l.begin(); it != l.end(); ++it) {
	 	(*it)->patch(i);
	 }

	return;
}

void TargetCode::printOut() {
	for (int i=0; i< 999 && codeArray[i] != NULL; i++) {
		cout << codeArray[i] << "\n";
	}
}

/* An abstraction for the Symbol Table 
 */
// class SymTbl {
// private:
// 	//Memory mem = Memory.getInstance();
// public:
// 	SymTbl() {}
// 	virtual void* get(const char* lexeme) = 0;
// 	virtual void put(const char* lexeme) = 0;
// };

/** Returns an entry from the Symbol table, using a lexeme (a string) as the key.
 *  In this simple implementation, it just falls back to the 1-char lexeme assumption
 *  (only the first char of the string is used)
 */
VarAddress* SimpleArraySymTbl::get(const char* lexeme) {
	return get(lexeme[0]);
}	

/** Returns an entry from the Symbol table, assuming that all lexemes are just 1-char long
 */
VarAddress* SimpleArraySymTbl::get(char lexeme) {
	int index = lexeme - 'a';

	return sym[index];
}

/** Stores an entry in the Symbol table, using a lexeme (a string) as the key
 *  In this simple implementation, it just falls back to the 1-char lexeme assumption
 *  (only the first char of the string is used)
 */
void SimpleArraySymTbl::put(const char* lexeme, typeName type) {
	put(lexeme[0], type);
}

/** Stores an entry in the Symbol table, assuming that all lexemes are just 1-char long
 */
void SimpleArraySymTbl::put(char lexeme, typeName type) {
	int index = lexeme -'a';

	int offset;

	// we store variables in memory, initializing them with a default value depending on their type
	switch(type) {
		case intType: {
			int intVal = 0;
			offset = mem.store(&intVal, sizeof(int));
			}
			break;
		case floatType: {
			float floatVal = 0;
			offset = mem.store(&floatVal, sizeof(float));
			}
			break;
		default:
			break;		
	}

	VarAddress* a = new VarAddress(lexeme, type, offset);
	sym[index] = a;
}

void SimpleArraySymTbl::printOut() {
	for (int i = 0; i < 26; ++i)
	{
		if (sym[i] != NULL) {
			switch (sym[i]->getType()) {
				case intType:
					cout << i << ") : " << sym[i] << " (int)   - offset = " << sym[i]->getOffset() << endl;
					break;
				case floatType:
					cout << i << ") : " << sym[i] << " (float) - offset = " << sym[i]->getOffset() << endl;
					break;
				default:
					/* should not occur */
					cout << i << ") : " << sym[i] << endl;
					break;
			}
		}
	}
}

/* TacInstr
 */
oprEnum TacInstr::getOp() const {
	return op;
}

void TacInstr::setValueNumber(int vn) {
	valueNumber = new InstrAddress(vn);
}

TacInstr::TacInstr(oprEnum op, Address* operand1, Address* operand2, TempAddress* temp) {
	this->valueNumber = NULL;
	this->op = op;
	this->operand1 = operand1;
	this->operand2 = operand2;

	this->temp = temp;
}

InstrAddress* TacInstr::getValueNumber() {
	return valueNumber;
}

// for backpathcing "goto"-like instructions
void TacInstr::patch(TacInstr* i) {
	assert(this->getOp() == jmpOpr || this->getOp() == condJmpOpr);

	this->operand1 = i->getValueNumber();
}


/*******************************/
/* ATTRIBUTES FOR NONTERMINALS */
/*******************************/

/** Constructor for ExprAttr, when the expression actually refers to an instruction
 */
ExprAttr::ExprAttr(TacInstr* addr, typeName type) {
	this->addr = addr->getValueNumber();
	this->type = type;
}

/** Constructor for ExprAttr, when the expression refers to a variable in the symbol table
 */
ExprAttr::ExprAttr(VarAddress* addr) {
	this->addr = addr;
	this->type = addr->getType();
}

/** Constructor for ExprAttr, when the expression refers to a constant
 */
ExprAttr::ExprAttr(ConstAddress* addr) {
	this->addr = addr;	
	this->type = addr->getType();
}

Address* ExprAttr::getAddr(){
	return addr;
}

/* BoolAttr
*/
void BoolAttr::addTrue(TacInstr* instr) {
	// check: must be a goto
	assert(instr->getOp() == jmpOpr || instr->getOp() == condJmpOpr);

	truelist.push_back(instr);
}

void BoolAttr::addFalse(TacInstr* instr) {
	// check: must be a goto
	assert(instr->getOp() == jmpOpr || instr->getOp() == condJmpOpr);

	falselist.push_back(instr);
}

void BoolAttr::addTrue(list<TacInstr*> l) {
	// Note: I should also check here for "goto" only
	truelist.merge(l);
}

void BoolAttr::addFalse(list<TacInstr*> l) {
	// Note: I should also check here for "goto" only
	falselist.merge(l);
}

list<TacInstr*> BoolAttr::getTruelist() {
	return truelist;
}

list<TacInstr*> BoolAttr::getFalselist() {
	return falselist;
}

/* StmtAttr
*/
void StmtAttr::addNext(TacInstr* instr) {
	nextlist.push_back(instr);
}

void StmtAttr::addNext(list<TacInstr*> l) {
	nextlist.merge(l);
}

list<TacInstr*> StmtAttr::getNextlist() {
	return nextlist;
}


/********************/
/* PRINTOUT METHODS */
/********************/
std::ostream& operator<<(std::ostream &out, const Address *addr) {
	return out << addr->toString();
}

std::ostream& operator<<(std::ostream &out, const InstrAddress *addr) {
	return out << addr->arrayCodeIndex;
}

std::ostream& operator<<(std::ostream &out, const TacInstr *instr) {
	// if (instr->operand1 != NULL && instr->operand2 != NULL) {
	//  	return out << setw(4) << instr->valueNumber << ": " << setw(7) << opTable[instr->op] << " " << setw(7) << instr->operand1 << " " << setw(7) << instr->operand2;
	//  } else if (instr->operand1 != NULL && instr->operand2 == NULL) {
	//    	return out << setw(4) << instr->valueNumber << ": " << setw(7) << opTable[instr->op] << " " << setw(7) << instr->operand1 << " " << setw(7) << "_";
	//  } else if (instr->operand1 == NULL && instr->operand2 != NULL) {
	//  	return out << setw(4) << instr->valueNumber << ": " << setw(7) << opTable[instr->op] << setw(7) << "_" << " " << setw(7) << instr->operand2;
	//  } else {
	//  	return out << setw(4) << instr->valueNumber << ": " << setw(7) << opTable[instr->op] << "       _       _";
	//  }
	switch(instr->getOp()) {
		case copyOpr:
			assert(instr->operand1 != NULL);
			if (instr->operand2 == NULL) {
				return out << setw(4) << instr->valueNumber << ": " << "t" << instr->valueNumber << " = " << instr->operand1;
			} else {
				return out << setw(4) << instr->valueNumber << ": " <<  instr->operand1 << " = " << instr->operand2;
			}
		case fakeOpr:
			return out << setw(4) << instr->valueNumber << ": " << opTable[instr->op];
		case jmpOpr:
			assert(instr->operand1 != NULL);
			return out << setw(4) << instr->valueNumber << ": " << opTable[instr->op] << " " << instr->operand1;			
		case addOpr:
			assert(instr->operand1 != NULL && instr->operand2 != NULL && instr->temp != NULL);
			return out << setw(4) << instr->valueNumber << ": " << instr->temp << " = " << instr->operand1 << " " << opTable[instr->op] << " " << instr->operand2;
		case haltOpr:
			return out << setw(4) << instr->valueNumber << ": " << opTable[instr->op];
		case mulOpr: /* TBD */
		case condJmpOpr: /* TBD */
		case UNKNOWNOpr: /* TBD */
		default:
			return out << setw(4) << instr->valueNumber << ": " << "???";
	}
	}
}
