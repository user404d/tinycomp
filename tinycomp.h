
#ifndef TINYCOMP_H_
#define TINYCOMP_H_

/* Type system */
typedef enum {intType, floatType} typeName;

/* Enums for 3-addr code - operators */
typedef enum { UNKNOWNOpr, copyOpr, addOpr, mulOpr, jmpOpr, condJmpOpr, fakeOpr} oprEnum;

class Attribute {

};

#endif