#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include <assert.h>

#include "phase10.h"


/*
TODO: Header comment.
*/
// fileScope is the name scope for the entire file
HWC_Part *semPhase10_part(PT_part_decl *parsedPart, HWC_NameScope *fileScope)
{
	HWC_Part *retval = malloc(sizeof(HWC_Part));
	if (retval == NULL)
	{
		assert(0);   // TODO: debug message
		return NULL;
	}
	memset(retval, 0, sizeof(*retval));

	retval->phases_completed = retval->phases_begun = 10;

	// Create publicNames, and make "NULL" its parent
	retval->publicNames = nameScope_malloc(NULL);
	if (retval->publicNames == NULL)
	{
		assert(0);   // TODO: debug message
		free(retval);
		return NULL;
	}
	// Create privateNames, and make "fileScope" its parent
	retval->privateNames = nameScope_malloc(fileScope);
	if (retval->privateNames == NULL)
	{
		assert(0);   // TODO: debug message
		free(retval->publicNames); // TODO: Is this right?
		free(retval);
		return NULL;
	}

	HWC_Decl *decl_list_head = NULL;
	// Implementation can be found in stmts.c
	// Creates an ordered (? Not necessary) list of decls within this part.
	retval->decls_len = extractHWCdeclsFromPTstmts(parsedPart->stmts, decl_list_head, retval->publicNames, retval->privateNames);

	// Implementation can be found in stmts.c
	// Converts the list of pt_stmts generated by the gramamr into HWC_Stmts
	retval->stmts_len = convertPTstmtIntoHWCstmt(parsedPart->stmts, retval->stmts);

	return retval;
}

