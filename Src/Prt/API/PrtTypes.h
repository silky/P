/**
* \file PrtTypes.h
* \brief Functions and data structures for P types.
* The P type system supports complex types. Each type has a kind (PRT_TYPE_KIND) and a
* type is a pointer to a PRT_TYPE_KIND. Once the kind of the type is known, then 
* it can be safely cast into a specific structure giving the details of that type.
*
* 1. **Constructing types**
*    + Static construction - Statically allocate the structs in this file. Do not free them.
*    + Dynamic construction - Use the MkX() functions to dynamically construct types. You must free the result using PrtFreeType().
*
* 2. **Foreign types**
*    + Purpose - To allow values that cannot be understood by P to be moved through the runtime.
*    + Type tag - A non-zero GUID to let the client code determine the type of a foreign value.
*    + Functions - A set of minimal functions to let PRT manipulate foreign values.
*/
#ifndef PRTTYPES_H
#define PRTTYPES_H

#ifdef __cplusplus
extern "C"{
#endif

/** The maximum length of a field name must be smaller than this constant */
#define PRT_MAXFLDNAME_LENGTH 256
#include "PrtConfig.h"

/** Structure for representing a standard GUID */
typedef struct PRT_GUID
{
	PRT_UINT32 data1;   /**< 0 First data field (32 bits)  */
	PRT_UINT16 data2;   /**< 0 Second data field (16 bits) */
	PRT_UINT16 data3;   /**< 0 Third data field (16 bits)  */
	PRT_UINT64 data4;   /**< 0 Fourth data field (64 bits) */
} PRT_GUID;

/** 
* \enum
* These are the kinds of types in the P type system. 
**/
typedef enum PRT_TYPE_KIND
{
	PRT_KIND_ANY    = 0,   /**< The kind of the `any` type        */
	PRT_KIND_BOOL   = 1,   /**< The kind of the `bool` type       */
	PRT_KIND_EVENT  = 2,   /**< The kind of the `event` type      */
	PRT_KIND_FORGN  = 3,   /**< The kind of all foreign types     */
	PRT_KIND_MACHINE    = 4,   /**< The kind of the `MACHINE` type         */
	PRT_KIND_INT    = 5,   /**< The kind of the `int` type        */
	PRT_KIND_MAP    = 6,   /**< The kind of all map types         */
	PRT_KIND_NMDTUP = 7,   /**< The kind of all named tuple types */
	PRT_KIND_NULL = 8,	   /**< The kind of the `NULL` type		  */
	PRT_KIND_SEQ    = 9,   /**< The kind of all sequence types    */
	PRT_KIND_TUPLE  = 10,  /**< The kind of all tuple types       */
	PRT_TYPE_KIND_COUNT = 11,   /**< The number of type kinds        */
	PRT_TYPE_KIND_CANARY = 0xFF /**< A freed type will have this as its kind */
} PRT_TYPE_KIND;

/** 
* \struct
* A type is expressed as a struct.
* More information for complex types are stored as discriminicated union 
**/
typedef struct PRT_TYPE {
	PRT_TYPE_KIND typeKind;             /**< The type kind      */
	union {
		struct PRT_MAPTYPE *map;		/**< Map type		    */
		struct PRT_NMDTUPTYPE *nmTuple;	/**< Named Tuple type	*/
		struct PRT_SEQTYPE *seq;		/**< Sequence type		*/
		struct PRT_TUPTYPE *tuple;		/**< Tuple type		    */
		struct PRT_FORGNTYPE *forgn;	/**< Foreign type	    */
	} typeUnion;
} PRT_TYPE;

/** 
* \struct 
* The layout for map types. 
*/
typedef struct PRT_MAPTYPE
{       
	PRT_TYPE *domType;   /**< The domain type of the map type */  
	PRT_TYPE *codType;   /**< The codomain type of the map type */
} PRT_MAPTYPE;

/** 
* \struct
* The layout for named tuple types. 
*/
typedef struct PRT_NMDTUPTYPE
{
	PRT_UINT32    arity;         /**< Arity of tuple type; arity > 0 */
	PRT_STRING    *fieldNames;   /**< Array of valid field names; length = arity */
	PRT_TYPE      **fieldTypes;  /**< Array of field types; length = arity */
} PRT_NMDTUPTYPE;

/** 
* \struct 
* The layout for sequence types. 
*/
typedef struct PRT_SEQTYPE
{
	PRT_TYPE *innerType;  /**< Inner type of the sequence */
} PRT_SEQTYPE;

/** 
* \struct 
* The layout for tuple types. 
*/
typedef struct PRT_TUPTYPE
{
	PRT_UINT32    arity;         /**< Arity of tuple type; arity > 0 */
	PRT_TYPE      **fieldTypes;   /**< Array of field types; length = arity */
} PRT_TUPTYPE;

/** The PRT_FORGN_CLONE function is called whenever a foreign value needs to be cloned.
*   The cloning semantics depends on the memory management strategy of the client.
*   @see PRT_FORGN_FREE
*/
typedef void*(PRT_CALL_CONV *PRT_FORGN_CLONE)(_In_ PRT_GUID typeTag, _In_  void *frgnVal);

/** The PRT_FORGN_FREE function is called whenever a foreign value will never be used again.
*   The semantics of PRT_FORGN_FREE depends on the memory management strategy of the client.
*   @see PRT_FORGN_CLONE
*/
typedef void(PRT_CALL_CONV *PRT_FORGN_FREE)(_In_ PRT_GUID typeTag, _Inout_ void *frgnVal);

/** The PRT_FORGN_GETHASHCODE function is called to get a hashcode for a foreign value.
*   The semantics depends of the client's definition of value equality. If two values
*   are equal, then the function must return the same hashcode.
*   @see PRT_FORGN_GETHASHCODE
*/
typedef PRT_UINT32(PRT_CALL_CONV *PRT_FORGN_GETHASHCODE)(_In_ PRT_GUID typeTag, _In_ void* frgnVal);

/** The PRT_FORGN_ISEQUAL function tests if two values are equal.
*   Equality semantics is determined by the client. If two values
*   are equal, then they should also have the same hashcode.
*   @see PRT_FORGN_GETHASHCODE
*/
typedef PRT_BOOLEAN(PRT_CALL_CONV *PRT_FORGN_ISEQUAL)(
	_In_ PRT_GUID typeTag1,
	_In_ void *frgnVal1,
	_In_ PRT_GUID typeTag2,
	_In_ void *frgnVal2);

/** 
* \struct 
* The layout for foreign types 
*/
typedef struct PRT_FORGNTYPE
{
	PRT_GUID              typeTag;    /**< A non-zero GUID used by the client to tag the foreign types of values */
	PRT_FORGN_CLONE       cloner;     /**< Clones foreign values */
	PRT_FORGN_FREE        freer;      /**< Frees foreign values */
	PRT_FORGN_GETHASHCODE hasher;     /**< Hashes foreign values */
	PRT_FORGN_ISEQUAL     eqTester;   /**< Tests foreign values for equality */
} PRT_FORGNTYPE;

/** Makes a foreign type.
* @param[in] typeTag The type tag for this type.
* @param[in] cloner The cloner for this type.
* @param[in] freer The freer for this type.
* @param[in] hasher The hasher for this type.
* @param[in] eqTester The equality tester for this type.
* @returns An instance of the foreign type. Caller is responsible for freeing.
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkForgnType(
	_In_ PRT_GUID              typeTag,
	_In_ PRT_FORGN_CLONE       cloner,
	_In_ PRT_FORGN_FREE        freer,
	_In_ PRT_FORGN_GETHASHCODE hasher,
	_In_ PRT_FORGN_ISEQUAL     eqTester);

/** Makes an instance of a primitive type.
* @param[in] primType Any primitive type; cannot be a foreign type.
* @returns An instance of a primitive. Caller is responsible for freeing.
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkPrimitiveType(_In_ PRT_TYPE_KIND primType);

/** Makes the absent type. 
* The absent type is a foreign type that represents the absence of a foreign value.
* It has a single foreign value, NULL, and is the default value of `foreign`.
* @returns An instance of the absent type. Caller is responsible for freeing.
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkAbsentType();

/** Makes a map type. 
* @param domType The domain type (will be deeply cloned).
* @param codType The codomain type (will be deeply cloned).
* @returns An instance of a map type. Caller is responsible for freeing.
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkMapType(_In_ PRT_TYPE *domType, _In_ PRT_TYPE *codType);

/** Makes a named tuple type with arity. Caller must fill in field names / types. 
* @param[in] arity The arity of the tuple type; arity > 0.
* @returns A named tuple type with space for field names and types. Caller is responsible for freeing.
* @see PrtSetFieldName
* @see PrtSetFieldType
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkNmdTupType(_In_ PRT_UINT32 arity);

/** Makes a sequence type.
* @param innerType The type of sequence's elements (will be deeply cloned).
* @returns An instance of a sequence type. Caller is responsible for freeing.
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkSeqType(_In_ PRT_TYPE *innerType);

/** Makes a tuple type with arity. Caller must fill in field types.
* @param[in] arity The arity of the tuple type; arity > 0.
* @returns A tuple type with space for field types. Caller is responsible for freeing.
* @see PrtSetFieldType
* @see PrtFreeType
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtMkTupType(_In_ PRT_UINT32 arity);

/** Sets the ith field type of a (named) tuple type. 
* @param[in,out] tupleType The (named) tuple type to mutate.
* @param[in] index The field index to set.
* @param[in] fieldType The type of the ith field (will be deeply cloned).
*/
PRT_API void PRT_CALL_CONV PrtSetFieldType(_Inout_ PRT_TYPE *tupleType, _In_ PRT_UINT32 index, _In_ PRT_TYPE *fieldType);

/** Sets the ith field name of a named tuple type.
* @param[in,out] tupleType The named tuple type to mutate.
* @param[in] index The field index to set.
* @param[in] fieldName The name of the ith field (will be deeply cloned).
*/
PRT_API void PRT_CALL_CONV PrtSetFieldName(_Inout_ PRT_TYPE *tupleType, _In_ PRT_UINT32 index, _In_ PRT_STRING fieldName);

/** Determines if subType and supType are in a sub-type / super-type relationship.
* @param[in] subType The sub-type.
* @param[in] supType The super-type.
* @returns `true` if subType is a sub-type of supType; `false` otherwise.
*/
PRT_API PRT_BOOLEAN PRT_CALL_CONV PrtIsSubtype(_In_ PRT_TYPE *subType, _In_ PRT_TYPE *supType);

/** Performs a deep clone of type. 
* @param[in] type The type to clone.
* @returns A deep clone of type. Caller is responsible for freeing. 
*/
PRT_API PRT_TYPE * PRT_CALL_CONV PrtCloneType(_In_ PRT_TYPE *type);

/** Recursively frees a type expression. Should only be called on types created using PrtMkXType()
* @param[in,out] type The type to free.
* @see PrtMkPrimitiveType
* @see PrtMkForgnType
* @see PrtMkMapType
* @see PrtMkNmdTupType
* @see PrtMkSeqType
* @see PrtMkTupType
*/
PRT_API void PRT_CALL_CONV PrtFreeType(_Inout_ PRT_TYPE *type);

/** Shallow test that type members are non-null.
* @param[in] type The type to check.
*/
PRT_API PRT_BOOLEAN PRT_CALL_CONV PrtIsValidType(_In_ PRT_TYPE *type);

#ifdef __cplusplus
}
#endif
#endif