#include "decl.h"

#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#include "part.h"

#include "wiring/fileRange.h"



/*
Given a list of PT_stmts, extracts all PT_decls and converts them into HWC_Decls.
This is done in a separate step from all other HWC_Stmts because decls are added to the namescope of the part/plugtype.
Returns an int corresponding to the length of the HWC_Decl array malloc'd in "output".
*/
int extractHWCdeclsFromPTstmts(PT_stmt *input, HWC_Decl **output, HWC_NameScope *publ, HWC_NameScope *priv)
{
	PT_stmt *currPTstmt = input;
	int len = 0;

	while(currPTstmt != NULL)
	{
		if(currPTstmt->mode == STMT_DECL)
		{
			// Nested while() because PT_decls have their own list of decls.
			PT_decl *currPTdecl = currPTstmt->stmtDecl;
			while(currPTdecl != NULL)
			{
				len++;
				currPTdecl = currPTdecl->prev;
			}
		}
		// TODO: Can remove this check if the program runs too slow, since the grammar ensures that the only stmts in plugtypes are decls.
		// Use the fact that PlugTypes have no private statements to check if the caller is a PlugType
		else if(priv == NULL)
		{
			// The grammar should prevent non-decl statements from being found in plugtypes, but check just in case.
			fprintf(stderr, "Statement that isn't a declaration found in a plugtype. Should be impossible, but obviously isn't. Crashing.\n");
			assert(0);
		}
		currPTstmt = currPTstmt->prev;
	}

	*output = malloc(sizeof(HWC_Decl)*len);
	if(*output == NULL)
	{
		assert(0); // TODO: Better error message?
	}

	// Reset to beginning of list
	currPTstmt = input;
	// Iterate through the backwards list again, but use "count" to write the output in forward order.
	int count = len-1;
	while(currPTstmt != NULL)
	{
		HWC_Decl *currHWCdecl = (*output)+count;
		if(currPTstmt->mode == STMT_DECL)
		{
			// TODO: Check if this code writes: [bit a, b, c] backwards or forwards
			PT_decl *currPTdecl = currPTstmt->stmtDecl;
			while(currPTdecl != NULL)
			{
				convertPTdeclIntoHWCdecl(currPTdecl, currHWCdecl);
				HWC_Nameable *thing = malloc(sizeof(HWC_Nameable));
				fr_copy(&thing->fr, &currHWCdecl->fr);
				thing->decl = currHWCdecl;
				// 1st check is for Parts    , makes sure the stmt is public
				// 2nd check if for PlugTypes, makes all decls public
				if(currPTstmt->isPublic == 1 || priv == NULL)
					nameScope_add(publ, currPTdecl->name, thing);
				else
					nameScope_add(priv, currPTdecl->name, thing);

				count--;
				currPTdecl = currPTdecl->prev;
			}
		}
		currPTstmt = currPTstmt->prev;
	}

	// Make sure that, by the end of this, the last index count wrote to was 0
	// +1 to offset the subtraction done after writing to 0
	assert(count+1 == 0);

	return len;
}


/*
Converts PT decls into HWC decls. What a good function name.

 - *input is a pointer to the PT_decl to convert
 - **output_out is a non-initialized HWC_Decl that this function will fill in

Returns nothing, since all meaningful work is done upon *output
*/
void convertPTdeclIntoHWCdecl(PT_decl *input, HWC_Decl *output)
{
	fr_copy(&output->fr, &input->fr);

	// Extract the "type" of the decl. See pt/type.h for details on what a type can be.
	PT_expr *convert = input->type;

	// Make sure convert has a proper mode.
	switch (convert->mode)
	{
		default:
			fprintf(stderr, "Bad mode! Value of [%d].\n", convert->mode);
			assert(0);
			break;

		case EXPR_BIT_TYPE:
		case EXPR_ARR:
		case EXPR_IDENT:
			break;
	}


	// If this is an array declaration, extract the length of the array.
	// Then, we'll update the 'convert' pointer, so that we can focus on
	// the base type and do the conversion
	if(convert->mode == EXPR_ARR)
	{
		convertPTexprIntoHWCexpr(convert->indexExpr, &output->expr);
		convert = convert->arrayExpr;
	}
	else
		output->expr = NULL;


	// TODO: Allow multi-level arrays
	// ie, this breaks when the "type" of an array is another array.
	if(convert->mode == EXPR_ARR)
	{
		fprintf(stderr, "Multi-level arrays are currently not supported in HWC.\n");
		assert(0);
	}


	// Note: We don't set *base_plugType or *base_part yet. Set type so we can check their validity later.
	//   We could check for and set decls of type "bit" here, but we'll need to check the part/plug name
	//   later anyhow, so pass on it for now.
	output->type     = convert->mode;
	output->typeName = convert->name;   // used for IDENT, ignored for BIT_TYPE

	output->isMem    = input->isMem;

	output->base_plugType = NULL;
	output->base_part     = NULL;

	// Temp dummy value that might be useful later
	output->indexSize = -1;
	output->indexMemory = -1;
}


/*
 * Ensures that the given decl's name hasn't already been used within its namescope.
 * 
 *  - *currDecl is the decl whose name will be checked
 *  - *currScope is the relevant namescope for this decl
 *  - isWithinPlug is used for a special check: Whether a part has been declared within a plugtype.
 *    - A decl within a part might refer to either a part or a plugtype
 *    - A decl within a plugtype must only refer to other plugtypes
 *    - isWithinPlug == 1 if the decl is within a plugtype, 0 if not.
 * 
 * Returns 0 if no errors, 1 if errors.
 *     (This will print out an appropriate error message, so the caller should
 *      not print anything out, but it *should* terminate with an error to its
 *      own caller.)
 */
int checkDeclName(HWC_Decl *currDecl, HWC_NameScope *currScope, int isWithinPlug)
{
	HWC_Nameable *currName;
	switch (currDecl->type)
	{
		default:
			fprintf(stderr, "currDecl->type == %d\n", currDecl->type);
			fprintf(stderr, "currDecl->typeName == %s\n", currDecl->typeName);
			fprintf(stderr, "currDecl was not of type bit, array, or ident.\n");
			assert(0);
			break;

		case EXPR_BIT_TYPE:
			currDecl->base_plugType = &BitType;
			break;

		case EXPR_ARR:
			fprintf(stderr, "-- TODO: %s(): Implement array types.\n", __func__);
			return 1;

		case EXPR_IDENT:
			// Does the type name exist in our current NameScope?
			currName = nameScope_search(currScope, currDecl->typeName);
			if(currName == NULL)
			{
				fprintf(stderr, "%s:%d:%d: Symbol '%s' not found\n",
				        currDecl->fr.filename,
				        currDecl->fr.s.l, currDecl->fr.s.c,
				        currDecl->typeName);
				return 1;
			}

			// Make sure the name we get back is either a plugtype or a part
			if(currName->plugtype == NULL && currName->part == NULL)
			{
				fprintf(stderr, "%s:%d:%d: Symbol '%s' is neither a part nor a plugtype.\n",
				        currDecl->fr.filename,
				        currDecl->fr.s.l, currDecl->fr.s.c,
				        currDecl->typeName);
				return 1;
			}

			// Check to make sure a Part declaration isn't inside a plugtype
			if(isWithinPlug == 1 && currName->part != NULL)
			{
				fprintf(stderr, "%s:%d:%d: Parts may not be fields inside plugtypes.\n",
				        currDecl->fr.filename,
				        currDecl->fr.s.l, currDecl->fr.s.c);
				return 1;
			}

			// What currDecl field to fill in?
			if(currName->part != NULL)
				currDecl->base_part     = currName->part;
			else
				currDecl->base_plugType = currName->plugtype;

			break;
	}
	return 0;
}


/*
TODO: Add header comment
Include that, by this point, we know either base_part or base_plugType should be filled in
   - isWithinPlug == 1 if the decl is within a plugtype, 0 if not.
*/
int findDeclSize(HWC_Decl *input, int isWithinPlug, int *numMemory)
{
	if(input->isMem == 1)
	{
		input->indexMemory = *numMemory;
		*numMemory += 1;
	}

	if(input->base_plugType != NULL)
	{
		// TODO: Fix inconsistent capitalization
		semPhase30_plugtype(input->base_plugType);
		return input->base_plugType->size;
	}
	else if(input->base_part != NULL)
	{
		if(isWithinPlug == 1)
		{
			// TODO: Error message
			// Plugtypes cannot contain parts
		}
		else
		{
			semPhase30_part(input->base_part);
			return input->base_part->size;
		}
	}
	else
	{
		// TODO: Error message
		// Likely a compiler error than a user error, since these fields should have been filled in phase20
	}

	return -1;
}
