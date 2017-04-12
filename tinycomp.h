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

/** Type system; each native data type is stored as a value in this enumeration.
 *  Note that for structured types we need a more complex structure; also, I am
 *  not explicitly accounting for a type hierarchy here.
 */
typedef enum {
	intType, 	/*!< integer type */
	floatType	/*!< floating point type */
} typeName;

/** Enums for 3-addr code - operators */
typedef enum {
	UNKNOWNOpr, /*!< this is the default, for an unknown operator (it should not occur) */
	haltOpr, 	/*!< return control to the operating system */
	copyOpr, 	/*!< the assignment operator */
	addOpr, 	/*!< the addition operator */
	mulOpr, 	/*!< the multiplication operator */
	indexCopyOpr, 	/*!< the indexed copy operator x[i] = y */
	offsetOpr, 	/*!< the displacement operator x = y[i] */
	jmpOpr, 	/*!< unconditional jump; the goto operator */
	condJmpOpr, /*!< conditional jump; the if ... goto operator */
	fakeOpr		/*!< a temporary "fake" operator for simulating the ones yet-to-be implemented */
} oprEnum;

/** An empty class representing the attributes of the grammar symbols.
 * It must be specialized for each specific attribute.
 */
class Attribute {
	/* intentionally empty */
};

#endif
