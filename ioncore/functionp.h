/*
 * wmcore/functionp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_FUNCTIONP_H
#define WMCORE_FUNCTIONP_H

#include "function.h"

#define FN_(CALLHND, ARGTSTR, OBJT, NAME, FUN) \
	{callhnd_##CALLHND, &OBJDESCR(OBJT), ARGTSTR, NAME, (void*)FUN}

#define FN(ARGT, HND, OBJT, NAME, FUN) \
	FN_(HND##_##ARGT, #ARGT, OBJT, NAME, FUN)

#define FN_VOID(HND, OBJT, NAME, FUN) \
	FN_(HND##_void, "", OBJT, NAME, FUN)

#define FN_GLOBAL(ARGT, NAME, FUN) \
	FN(ARGT, global, WObj, NAME, FUN)

#define FN_GLOBAL_VOID(NAME, FUN) \
	FN_VOID(global, WObj, NAME, FUN)

#define FN_SCREEN(ARGT, NAME, FUN) \
	FN(ARGT, generic, WScreen, NAME, FUN)

#define FN_SCREEN_VOID( NAME, FUN) \
	FN_VOID(generic, WScreen, NAME, FUN)

#define END_FUNTAB {NULL, NULL, NULL, NULL, NULL}


extern void callhnd_direct(WThing *thing, WFunction *func,
						   int n, const Token *args);
extern void callhnd_generic_void(WThing *thing, WFunction *func,
								 int n, const Token *args);
/* Notice: although the libtu token type l(ong) is used, functions
 * are expected to take an int argument.
 */
extern void callhnd_generic_l(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_generic_b(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_generic_d(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_generic_s(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_generic_ss(WThing *thing, WFunction *func,
							   int n, const Token *args);
extern void callhnd_global_void(WThing *thing, WFunction *func,
								int n, const Token *args);
extern void callhnd_global_l(WThing *thing, WFunction *func,
							 int n, const Token *args);
extern void callhnd_global_ll(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_global_s(WThing *thing, WFunction *func,
							 int n, const Token *args);
extern void callhnd_button_void(WThing *thing, WFunction *func,
								int n, const Token *args);
extern void callhnd_drag_void(WThing *thing, WFunction *func,
							  int n, const Token *args);
extern void callhnd_cclient_void(WThing *thing, WFunction *func,
								 int n, const Token *args);

#endif /* WMCORE_FUNCTIONP_H */
