/*
 * wmcore/hooks.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_HOOKS_H
#define WMCORE_HOOKS_H

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
	

#define ADD_HOOK(HOOK, FN) add_to_symlist(&HOOK, (void*)(FN))
#define REMOVE_HOOK(HOOK, FN) remove_from_symlist(&HOOK, (void*)(FN))

#endif /* WMCORE_HOOKS_H */
