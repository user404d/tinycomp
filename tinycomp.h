#ifndef TINYCOMP_H_
#define TINYCOMP_H_

/**
* @file tinycomp.h
* @brief This header file contains the definitions that must
* be shared between flex, bison, and the support code.
*
* @author Marco Ortolani
* @date 3/13/2017 
*/

/** Type system */
typedef enum {intType, floatType} typeName;

/** Enums for 3-addr code - operators */
typedef enum { UNKNOWNOpr, copyOpr, addOpr, mulOpr, jmpOpr, condJmpOpr, fakeOpr} oprEnum;

/** An empty class representing the attributes of the grammar symbols.
 * It must be specialized for each specific attribute.
 */
class Attribute {
	/* intentionally empty */
};

#endif