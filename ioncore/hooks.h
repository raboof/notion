/*
 * ion/ioncore/hooks.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_HOOKS_H
#define ION_IONCORE_HOOKS_H

/* The hooks in this file have nothing to do with the hook mechanism
 * in ../etc/ioncorelib.lua and only the "alternative" hooks are used
 * anymore for alternative handlers to low-level functions.
 */

#include "symlist.h"


typedef WSymlist WHooklist;


#define CALL_HOOKS(HOOK, ARGS)              \
	{                                       \
		typedef void HookFn();              \
		HookFn *hk;                         \
                                            \
		ITERATE_SYMLIST(HookFn*, hk, HOOK){ \
			hk ARGS;                        \
		}                                   \
	}


#define CALL_ALT_B(RET, ALT, ARGS)        \
	{                                     \
		typedef bool AltFn();             \
		AltFn *hk;                        \
		RET=FALSE;                        \
                                          \
		ITERATE_SYMLIST(AltFn*, hk, ALT){ \
			RET=hk ARGS;                  \
			if(RET)                       \
				break;                    \
		}                                 \
	}


#define CALL_ALT_B_NORET(ALT, ARGS)       \
	{                                     \
		typedef bool AltFn();             \
		AltFn *hk;                        \
                                          \
		ITERATE_SYMLIST(AltFn*, hk, ALT){ \
			if(hk ARGS)                   \
			    break;                    \
		}                                 \
	}


#define CALL_ALT_P(TYPE, RET, ALT, ARGS)  \
	{                                     \
		typedef TYPE *AltFn();            \
		AltFn *hk;                        \
		RET=NULL;                         \
                                          \
		ITERATE_SYMLIST(AltFn*, hk, ALT){ \
			RET=hk ARGS;                  \
			if(RET!=NULL)                 \
				break;                    \
		}                                 \
	}
	

#define ADD_HOOK(HOOK, FN) symlist_insert(&(HOOK), (void*)(FN))
#define REMOVE_HOOK(HOOK, FN) symlist_remove(&(HOOK), (void*)(FN))

#endif /* ION_IONCORE_HOOKS_H */
