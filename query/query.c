/*
 * ion/query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/exec.h>
#include <ioncore/clientwin.h>
#include <ioncore/focus.h>
#include <ioncore/names.h>
#include <ioncore/genws.h>
#include <ioncore/genframe.h>
#include <ioncore/reginfo.h>
#include <ioncore/extl.h>
#include "query.h"
#include "wedln.h"
#include "wmessage.h"
#include "fwarn.h"


/*{{{ Generic */


/*EXTL_DOC
 * Show a query window in \var{frame} with prompt \var{prompt}, initial
 * contents \var{dflt}. The function \var{handler} is called with
 * the entered string as the sole argument when \fnref{wedln_finish}
 * is called. The function \var{completor} is called with the created
 * \type{WEdln} is first argument and the string to complete is the
 * second argument when \fnref{wedln_complete} is called.
 */
EXTL_EXPORT
void query_query(WGenFrame *frame, const char *prompt, const char *dflt,
				 ExtlFn handler, ExtlFn completor)
{
	WRectangle geom;
	WEdln *wedln;
	WEdlnCreateParams fnp;
	
	/*fprintf(stderr, "query_query called %s %s %d %d\n", prompt, dflt,
			handler, completor);*/
	
	fnp.prompt=prompt;
	fnp.dflt=dflt;
	fnp.handler=handler;
	fnp.completor=completor;
	
	wedln=(WEdln*)genframe_add_input(frame, (WRegionAddFn*)create_wedln,
									 (void*)&fnp);
	if(wedln!=NULL){
		region_map((WRegion*)wedln);
		
		if(REGION_IS_ACTIVE(frame))
			set_focus((WRegion*)wedln);
	}
}


/*}}}*/


