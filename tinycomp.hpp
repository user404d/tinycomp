#ifndef TINYCOMP_HPP_
#define TINYCOMP_HPP_

/**
* @file tinycomp.hpp
* @brief This header file contains the support code for 
* the translator of tinycomp.
*
* @author Marco Ortolani
* @date 3/13/2017 
*/


#include <iostream>
#include <list>
#include "tinycomp.h"

using namespace std;

/* *************************/
/* REPRESENTING ADDRESSES  */
/* *************************/

/** A generic address for 3-addr code instructions. This can be:
 * - a constant
 * - a variable (from the symbol table)
 * - the address of an instruction (the famous valuenumber)
 */
class Address {
protected:
	/** Overloading of the << operator.
	 *  It will print the Address by way of the toString() method
	 */
	friend std::ostream& operator<<(std::ostream &, const Address *);

	/** Abstract method for printing an Address.
	 *  Note that toString() *must* be defined in derived classes.
	 */
	virtual const char* toString() const = 0;
};

/** A specialization of Address to hold a constant 
 */
class ConstAddress: public Address {
private:
	typeName type;
	union {
		int i;
		float f;
	} val;

public:
	/* Constructor for an int constant */
	ConstAddress(int i);

	/** Constructor for a float constant. */
	ConstAddress(float f); 

	const char* toString() const; 
};

/** A specialization of Address to hold a variable 
 */
class VarAddress: public Address {
private:
	char lexeme;
public:
	VarAddress(char v);

	const char* toString() const;
};

/** A specialization of Address to hold an instruction 
 */
class InstrAddress: public Address {
private:
	int arrayCodeIndex;

	friend std::ostream& operator<<(std::ostream &, const InstrAddress *);

public:
	InstrAddress(int vn);

	const char* toString() const;
};

/* **************/
/*  3-ADDR CODE */
/* **************/

/** A generic three-address code instruction.
 *  It will store:
 *  - the instruction's valuenumber
 *  - the operator \sa oprEnum
 */
class TacInstr {
private:
	InstrAddress* valueNumber; 
	oprEnum op;
	Address* operand1;
	Address* operand2;

	void setValueNumber(int vn);
	friend class TargetCode;

	friend std::ostream& operator<<(std::ostream &, const TacInstr *);
public:
	TacInstr(oprEnum op, Address* operand1, Address* operand2);

	oprEnum getOp() const;
	InstrAddress* getValueNumber();

	/** for backpathcing "goto"-like instructions */
	void patch(TacInstr*);
};

/* ***************************/
/*  COMPILER DATA STRUCTURES */
/* ***************************/

/** A simplified abstraction for the memory allocated to the compiler.
 *  More precisely: the stack
 */
class Memory {
private:
	// Private Constructor
	Memory();

	// Stop the compiler from generating methods of copy the object
    Memory(Memory const& copy);            // Not to be implemented
    Memory& operator=(Memory const& copy); // Not to be implemented
public:
    static Memory& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static Memory instance;
        return instance;
    }
};


/** A simplified abstraction for representing our target code.
 *  Following the textbook, I'm using 3-addr code instructions
 *  and storing them in an actual array.
 */
class TargetCode {
private:
	TacInstr* codeArray[1000];
	int nextInstr;

	TacInstr* gen(TacInstr* instr);
public:
	TargetCode();
	TacInstr* getInstr(int i);


	int getNextInstr();

	TacInstr* gen(oprEnum op, Address* operand1, Address* operand2);

	void backpatch(list<TacInstr*>, TacInstr*);
	void printOut();
};

/** An abstraction for the Symbol Table 
 */
class SymTbl {
private:
	//Memory mem = Memory.getInstance();
public:
	SymTbl() {}
	virtual void* get(const char* lexeme) = 0;
	virtual void put(const char* lexeme) = 0;
};

/** A simple implementation for a symbol table. 
 *  I assume here that var id's are 1-char long,
 *  so the table is just an array with 26 entries 
 */
class SimpleArraySyTbl : public SymTbl {
private:
	VarAddress *sym[26];
public:
	void* get(const char* lexeme);
	void* get(char lexeme);

	void put(const char* lexeme);
	void put(char lexeme);
};

/* ******************************/
/*  ATTRIBUTES FOR NONTERMINALS */
/* ******************************/

//// An abstract attribute
//class Attribute {
//
//};

/** expr: arithmetic expressions 
 * - E.addr
 */
class ExprAttr: public Attribute {
private:
	InstrAddress* addr;

public:
	ExprAttr(TacInstr* addr);

	Address* getAddr();
};

/** cond: boolean expressions 
 * - B.truelist
 * - B.falselist
 */
class BoolAttr: public Attribute {
private:
	list<TacInstr*> truelist;
	list<TacInstr*> falselist;
public:
	BoolAttr() {}

	void addTrue(TacInstr*);
	void addFalse(TacInstr*);

	// the following implement the "merge"
	void addTrue(list<TacInstr*>);
	void addFalse(list<TacInstr*>);

	list<TacInstr*> getTruelist();
	list<TacInstr*> getFalselist();
};

/** stmt: statements
 * - S.nextlist
 */
class StmtAttr: public Attribute {
private:
	list<TacInstr*> nextlist;
	list<TacInstr*> falselist;
public:
	StmtAttr() {}

	void addNext(TacInstr*);

	// the following implement the "merge"
	void addNext(list<TacInstr*>);

	list<TacInstr*> getNextlist();
};

#endif //TINYCOMP_H_