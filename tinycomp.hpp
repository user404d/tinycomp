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
	/** Constructor for an int constant */
	ConstAddress(int i);

	/** Constructor for a float constant. */
	ConstAddress(float f); 

	/** Concrete method for printing a ConstAddress;
	 *  it's a concrete implementation of the corresponding abstract method in Address
	 */
	const char* toString() const; 
};

/** A specialization of Address to hold a variable 
 */
class VarAddress: public Address {
private:
	char lexeme;
public:
	/** Constructor: creates a variable from its id (assuming only 1-char id's) */
	VarAddress(char v);

	/** Concrete method for printing a VarAddress;
	 *  it's a concrete implementation of the corresponding abstract method in Address
	 */
	const char* toString() const;
};

/** A specialization of Address to hold an instruction. 
 */
class InstrAddress: public Address {
private:
	int arrayCodeIndex;

	friend std::ostream& operator<<(std::ostream &, const InstrAddress *);

public:
	/** Constructor to initialize an InstrAddress from an index of the array code. 
	 *  @param vn The index of the TargetCode array, representing a valuenumber.
	 */
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
	/** Constructor of a 3-address code instruction. The result is internally stored 
	 * as an InstrAddress representing the value number (e.g corresponding to an index to the code array)
	 * @param op The operator for this instruction, as an oprEnum
	 * @param operand1 The first operand, as a generic Address
	 * @param operand2 The second operand (may be NULL for operators that do not require 2 operands)
	 */
	TacInstr(oprEnum op, Address* operand1, Address* operand2);

	/** Returns the enum representing the operator of this specific instruction */
	oprEnum getOp() const;

	/** Returns the InstrAddress representing the value number */	
	InstrAddress* getValueNumber();

	/** For backpathcing "goto"-like instructions */
	void patch(TacInstr*);
};

/* ***************************/
/*  COMPILER DATA STRUCTURES */
/* ***************************/

/** A simplified abstraction for the memory allocated to the compiler.
 */
class Memory {
private:
	/** Private Constructor
	 */
	Memory();

	// Stop the compiler from generating methods of copy the object
    Memory(Memory const& copy);            // Not to be implemented
    Memory& operator=(Memory const& copy); // Not to be implemented
public:
	/** As Memory is implemented as a singleton, its constructor is private.
	 *  This method is the only way to obtain an instance of Memory.
	 *  It is guaranteed that it will return always the same instance.
	 */
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
	/** Basic constructor; it will initialize the internal array of TacInstr instructions */
	TargetCode();

	/** Returns the instruction stored at index i in the code array */
	TacInstr* getInstr(int i);

	/** Implementation of "nextinstr" from the textbook */
	int getNextInstr();

	/** Implementation of "gen()" from the textbook.
	 *  Basically, if generates a new TacInstr with the given parameters, 
	 *  and stores it in the next available place in the code array
	 */
	TacInstr* gen(oprEnum op, Address* operand1, Address* operand2);

	/** Implementation of "backpatch()" from the textbook.
	 *  @param gotolist a list of TacInstr; each one is assumed to be a "goto"-like instruction
	 *  @param instr the Address of the instruction (i.e. TacInstr) to be patched in the goto's in the list
	 */
	void backpatch(list<TacInstr*> gotolist, TacInstr* instr);

	/** A convenience method to print out the entire code array */
	void printOut();
};

/** An abstraction for the Symbol Table 
 */
class SymTbl {
private:
	//Memory mem = Memory.getInstance();
public:
	SymTbl() {}

	/** Pure virtual method; retrieves a variable from the symbol table.
	 *  @param lexeme The lexeme used as a key to access the symbol table
	 */
	virtual void* get(const char* lexeme) = 0;

	/** Pure virtual method; stores a variable into the symbol table.
	 *  NEEDS TO BE MODIFIED.
	 *  @param lexeme The lexeme used as a key to access the symbol table
	 */
	virtual void put(const char* lexeme) = 0;
};

/** A simple implementation for a symbol table. 
 *  I assume here that var id's are 1-char long,
 *  so the table is just an array with 26 entries.
 *  NOTE: this class needs to be defined yet.
 */
class SimpleArraySyTbl : public SymTbl {
private:
	VarAddress *sym[26];
public:
	/** TBD */
	void* get(const char* lexeme);

	/** TBD */
	void* get(char lexeme);

	/** TBD */
	void put(const char* lexeme);

	/** TBD */
	void put(char lexeme);
};

/* ******************************/
/*  ATTRIBUTES FOR NONTERMINALS */
/* ******************************/

//// An abstract attribute
//class Attribute {
//
//};

/** Implementation of attribute for grammar symbol expr: arithmetic expressions 
 * - E.addr
 */
class ExprAttr: public Attribute {
private:
	InstrAddress* addr;

public:
	/** Constructor for ExprAttr; it will refer to the Address (valuenumber) of the instruction
	 *  that (when executed) will contain the result of the entire expression. 
	 */
	ExprAttr(TacInstr* addr);

	/** Returns the E.addr attribute; in fact an instance of InstrAddress. */
	Address* getAddr();
};

/** Implementation of attribute for grammar symbol cond: boolean expressions 
 * - B.truelist
 * - B.falselist
 */
class BoolAttr: public Attribute {
private:
	list<TacInstr*> truelist;
	list<TacInstr*> falselist;
public:
	BoolAttr() {}

	/** Appends a 3-addr code instruction to the truelist.
	 * 
	 *  @param instr The instruction to be appended; it is assumed to contain a "goto"-like operator.
	 */
	void addTrue(TacInstr* instr);

	/** Appends a 3-addr code instruction to the falselist.
	  * 
	  * @param instr The instruction to be appended; it is assumed to contain a "goto"-like operator.
	  */
	void addFalse(TacInstr*instr);

	/** Appends a list of instructions to the truelist.
	 *  Basically, an implementation of merge() for a truelist.
     *  @param l The list to be appended; it is assumed to contain only "goto"-like instructions.
	 *
	 */ 
	void addTrue(list<TacInstr*> l);

	/** Appends a list of instructions to the falselist.
	 *  Basically, an implementation of merge() for a falselist.
     *  @param l The list to be appended; it is assumed to contain only "goto"-like instructions.
	 *
	 */ 
	void addFalse(list<TacInstr*> l);

	/** Returns the truelist. */
	list<TacInstr*> getTruelist();

	/** Returns the falselist. */
	list<TacInstr*> getFalselist();
};

/** Implementation of attribute for grammar symbol stmt: a generic statement.
 * - S.nextlist
 */
class StmtAttr: public Attribute {
private:
	list<TacInstr*> nextlist;
public:
	StmtAttr() {}

	/** Appends an instruction to the nextlist */
	void addNext(TacInstr*);

	/** Appends a list of instructions to the next list.
	 *  Basically, an implementation of merge() for a nextlist.
     *  @param l The list to be appended; it is assumed to contain only "goto"-like instructions.
     */
	void addNext(list<TacInstr*> l);

	/** Returns the nextlist. */
	list<TacInstr*> getNextlist();
};

#endif //TINYCOMP_H_