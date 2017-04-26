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

#include<unordered_map>

/** Type system; each native data type is stored as a value in this enumeration.
 *  Note that for structured types we need a more complex structure; also, I am
 *  not explicitly accounting for a type hierarchy here.
 */
typedef enum typeTree {
  IDENTITY,     /*!< identity 0x0 = int ^ int, etc. */
  intType,	/*!< integer type 0x1 */
  fracType,     /*!< fraction type 0x2 */
  FRACPROMO,    /*!< promote to fraction 0x3 = int ^ fraction */
  floatType,	/*!< floating point type 0x4 */
  FLOATPROMO,   /*!< promote to float 0x5 = int ^ float */
  ERROR         /*!< Undefined type */
} typeName;

/** Enums for 3-addr code - operators */
typedef enum {
  UNKNOWNOpr,   /*!< this is the default, for an unknown operator (it should not occur) */
  haltOpr, 	/*!< return control to the operating system */
  copyOpr, 	/*!< the assignment operator */
  addOpr, 	/*!< the addition operator */
  mulOpr, 	/*!< the multiplication operator */
  divOpr,       /*!< the division operator */
  indexCopyOpr, /*!< the indexed copy operator x[i] = y */
  offsetOpr, 	/*!< the displacement operator x = y[i] */
  jmpOpr, 	/*!< unconditional jump; the goto operator */
  jeOpr,        /*!< jump if equal operator */
  condJmpOpr,   /*!< conditional jump; the if ... goto operator */
  fakeOpr	/*!< a temporary "fake" operator for simulating the ones yet-to-be implemented */
} oprEnum;

/** An empty class representing the attributes of the grammar symbols.
 * It must be specialized for each specific attribute.
 */
class Attribute {
  /* intentionally empty */
};

/** Fraction class */
class Fraction {
public:
  /** numerator */
  std::int32_t num,
  /** denominator */
               denom;

  /** Fraction default constructor.
   */
  Fraction() = default;

  /** Constructor for a Fraction.
   * @param _num an int for the numerator
   * @param _denom an int for the denominator
   */
 Fraction(std::int32_t _num, std::int32_t _denom)
   : num(_num), denom(_denom) {};
};

/** Namespace containing type lookup table.
 */
namespace Type
{
  /** Functor for typeName hashing.
   */
  struct type_name_hash
  {
    /** Implements hash function for typeName.
        @param val a typeName;
    */
    std::size_t operator()(const typeName val) const {
      return static_cast<std::size_t>(val);
    }
  };

  /** typedef for typeName -> size mapping. */
  using TypeSizeMap = std::unordered_map<typeName,std::size_t,type_name_hash>;

  /** Map<typeName,std::size_t> allows for runtime lookup of type widths */
  static const TypeSizeMap size =
    {
      {typeTree::intType, sizeof(int)},
      {typeTree::fracType, sizeof(Fraction)},
      {typeTree::floatType, sizeof(float)}
    };
}

#endif
