/*
 * wmcore/completehelp.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "completehelp.h"


bool add_to_complist(char ***cp_ret, int *n, char *name)
{
	char **cp;
	
	cp=REALLOC_N(*cp_ret, char*, *n, (*n)+1);
	
	if(cp==NULL){
		warn_err();
		return FALSE;
	}else{
		cp[*n]=name;
		(*n)++;
		*cp_ret=cp;
	}
	
	return TRUE;
}


bool add_to_complist_copy(char ***cp_ret, int *n, const char *nam)
{
	char *name=scopy(nam);
	bool ret;
	
	if(name==NULL){
		warn_err();
		return FALSE;
	}
	
	ret=add_to_complist(cp_ret, n, name);
	
	if(ret==FALSE)
		free(name);
	
	return ret;
}
