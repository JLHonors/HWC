#ifndef __SEMANTIC_PLUGTYPE_H__INCLUDED__
#define __SEMANTIC_PLUGTYPE_H__INCLUDED__


// Maybe just want to reference "plug" struct instead?
typedef struct plugtype plugtype;
struct plugtype
{
	/* these track the progress of the semantic phase.  The 'completed'
	 * field tells us which semantic phase is done for this type; calling
	 * a generator function on this type, for one of these phases, is a
	 * NOP.  The 'begun' field tells you what phases have begun, which is
	 * used to detect recursive types.
	 */
	int phases_completed;
	int phases_begun;

	// All names within a plugtype are public
	HWC_NameScope *publicNames;

	// Only stmts allowed in plugtypes are decls.
	HWC_Decl *decls;
	int       decls_len;

	// TODO: Field for sizeof Part
};

// Presumably put primitive "bit" here
/*
plugtype BitPrimitive;
BitPrimitive.wire = 0;
*/


#endif // __SEMANTIC_PLUGTYPE_H__INCLUDEDED__
