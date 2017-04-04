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

	/** Returns the constant's type (as a typeName enum)
	 */
	typeName getType();

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

	typeName type;
	int width;

	/* pointer to the memory, where the var value is stored */
	int offset;

public:
	/** Constructor: creates a variable address from its id (assuming only 1-char id's).
	 */
	VarAddress(char v, typeName t, int offset);

	/** Returns the variable's type (as a typeName enum)
	 */
	typeName getType();

	/** Returns the variable's width, which depends on its type
	 */
	int getWidth();

	/** Returns the pointer to the memory location holding the variable's value
	 */
	int getOffset();

	/** Concrete method for printing a VarAddress;
	 *  it's a concrete implementation of the corresponding abstract method in Address
	 */
	const char* toString() const;
};

/** A specialization of Address to hold a temporary 
 */
class TempAddress: public Address {
private:
	static int counter;

	int name;

	int offset;
public: 
	/** Constructor: creates a temporary at the specified offset in memory
	 */
	TempAddress(int offset);

	/** Returns the pointer to the memory location holding the temporary
	 */
	int getOffset();

	/** Concrete method for printing a TempAddress;
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

	TempAddress* temp;

	void setValueNumber(int vn);
	friend class TargetCode;

	friend std::ostream& operator<<(std::ostream &, const TacInstr *);
public:
	/** Constructor of a 3-address code instruction. The result is internally stored 
	 * as an InstrAddress representing the value number (e.g corresponding to an index to the code array)
	 * @param op The operator for this instruction, as an oprEnum
	 * @param operand1 The first operand, as a generic Address
	 * @param operand2 The second operand (may be NULL for operators that do not require 2 operands)
	 * @param temp Holds a temporary, when explicitly needed to specify the address result, depending on the operation
	 */
	TacInstr(oprEnum op, Address* operand1, Address* operand2, TempAddress* temp);

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
	/* our (simulation of the) actual memory */
	void* storage;

	/* the pointer to the next block of free memory */
	int offset; 

	/** Private Constructor
	 */
	Memory() {
		storage = (void*)malloc(1000000 * sizeof(void*) );
		offset = 0;
	}

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

    /** Store the bytes pointed to by val in memory.
     *  Note that we don't pass the type of the variable to be stored, as this 
     *  has no relevance for the memory.
     *
     *	Returns the *beginning* address of the value just stored.
     */
    int store(void* val, int width) {
    	/* offset tells us where free memory begins 
    	 * Unfortunaltely, arithmetic on void pointers is not allowed in C,
    	 * so I need to cast to char, and cast back
    	 */
    	void* begin = (void*)((char*)storage + offset);

    	memcpy(begin, val, width);

    	int oldoffset = offset;
    	offset += width;

    	return oldoffset;
    }

    /** Returns the *beginning* address of some value, supposedly stored in memory.
     *  Note that we have no clue about the type of such value, or it's width.
     *  They must be "computed/retrieved" externally.
     */
    void* retrieve(int offset) {
    	return (void*)((char*)storage + offset);
    }

    /** Returns a new temporary address pointing to the first location of available memory
     *  Since we would later need to advance the offset anyway, this methods takes care of this;
     *  that's why we pass the width of what we're gonna store in that location.
     *
     *  It returns the *beginning* address of the value to be stored therein (i.e. the address of the temporary)
     */
    TempAddress* getNewTemp(int width) {
	   	void* begin = (void*)((char*)storage + offset);

     	int oldoffset = offset;
    	offset += width;

    	return new TempAddress(oldoffset);
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

	/** Implementation of "gen()" from the textbook.
	 *  Basically, if generates a new TacInstr with the given parameters, 
	 *  and stores it in the next available place in the code array
	 *  This version accounts for using temporaries.
	 */
	TacInstr* gen(oprEnum op, Address* operand1, Address* operand2, TempAddress* temp);

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
protected:
	Memory& mem = Memory::getInstance();

public:
	SymTbl() {}

	/** Pure virtual method; retrieves a variable from the symbol table.
	 *  @param lexeme The lexeme used as a key to access the symbol table
	 */
	virtual VarAddress* get(const char* lexeme) = 0;

	/** Pure virtual method; stores a variable into the symbol table.
	 *  @param lexeme The lexeme used as a key to access the symbol table
	 */
	virtual void put(const char* lexeme, typeName type) = 0;
};

/** A simple implementation for a symbol table. 
 *  I assume here that var id's are 1-char long,
 *  so the table is just an array with 26 entries.
 *  NOTE: this class needs to be defined yet.
 */
class SimpleArraySymTbl : public SymTbl {
private:
	VarAddress *sym[26];
public:
	/** Returns an entry, indexed by its lexeme */
	VarAddress* get(const char* lexeme);

	/** Returns an entry, indexed by its lexeme (1-char version) */
	VarAddress* get(char lexeme);

	/** Stores a variable in the symbol table, given its lexeme and type  */
	void put(const char* lexeme, typeName type);

	/** Stores a variable in the symbol table, given its lexeme (1-char version) and type  */
	void put(char lexeme, typeName type);

	/** Returns the value of a variable by first recovering the offset, and then accessing the memory.
	 *  Since we don't know the type to be returned, a (void*) is used.
	 */
	void* getVarValue(char lexeme) {
		int off = sym[lexeme]->getOffset();
		return mem.retrieve(off);
	}

	/** Prints out the symbl table */
	void printOut();
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
	Address* addr;
	typeName type;

public:
	/** Constructor for ExprAttr; it will refer to the Address (valuenumber) of the instruction
	 *  that (when executed) will contain the result of the entire expression. 
	 *  It needs to know (and store) the type of the result.
	 */
	ExprAttr(TacInstr* addr, typeName type);

	/** Constructor for ExprAttr; it will refer to the Address of the variable. 
	 *  It will infer the type from the type of the variable.
	 */
	ExprAttr(VarAddress* addr);

	/** Constructor for ExprAttr; it will refer to a constant. 
	 *  It will infer the type from the type of the constant.
	 */
	ExprAttr(ConstAddress* addr);
	
	/** Constructor for ExprAttr; it will refer to a temporary,
	 *  supposedly holding some variable. 
	 *  The type cannot be inferred, in ths case, so it must be explicitly provided.
	 */
	ExprAttr(TempAddress* addr, typeName type);

	/** Returns the E.addr attribute */
	Address* getAddr();

	/** Returns the E.type attribute */
	typeName getType();
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