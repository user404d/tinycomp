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

/* 
 * VarAddress 
 */
VarAddress::VarAddress(char v) {
	lexeme= v;
}

const char* VarAddress::toString() const {
	char* str = (char*)malloc(2*sizeof(char));
	str[0] = lexeme;
	str[1] = '\0';

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

TacInstr* TargetCode::gen(oprEnum op, Address* operand1, Address* operand2) {
	return gen(new TacInstr(op, operand1, operand2));
}

void TargetCode::printOut() {
	for (int i=0; i< 1000 && codeArray[i] != NULL; i++) {
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

/* SimpleArraySyTbl
 */
void* SimpleArraySyTbl::get(const char* lexeme) {
	return get(lexeme[0]);
}	

void* SimpleArraySyTbl::get(char lexeme) {
	int index = lexeme - 'a';

	return sym[index];
}

void SimpleArraySyTbl::put(const char* lexeme) {
	put(lexeme[0]);
}

void SimpleArraySyTbl::put(char lexeme) {
	int index = lexeme -'a';

	VarAddress* a = new VarAddress(lexeme);
	sym[index] = a;
}

/* TacInstr
 */
oprEnum TacInstr::getOp() const {
	return op;
}

void TacInstr::setValueNumber(int vn) {
	valueNumber = new InstrAddress(vn);
}

TacInstr::TacInstr(oprEnum op, Address* operand1, Address* operand2) {
	this->valueNumber = NULL;
	this->op = op;
	this->operand1 = operand1;
	this->operand2 = operand2;
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

/* ExprAttr
*/
ExprAttr::ExprAttr(TacInstr* addr) {
	this->addr = addr->getValueNumber();
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
		case mulOpr:
		case condJmpOpr:
		case UNKNOWNOpr:
		default:
			return out << setw(4) << instr->valueNumber << ": " << "???";
	}
}

