/*
 * ion/ioncore/errorlog.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <libtu/util.h>

#include "common.h"
#include "errorlog.h"

static ErrorLog *current_log=NULL;

static void add_error(ErrorLog *el, const char *message)
{
	int l;
	
	if(message==NULL)
		return;
	
	if(el->file!=NULL){
		el->errors=TRUE;
		fwrite(message, sizeof(char), strlen(message), el->file);
		fputc('\n', el->file);
		return;
	}

	if(el->msgs==NULL){
		el->msgs=ALLOC_N(char, ERRORLOG_MAX_SIZE);
		if(el->msgs==NULL){
			fprintf(stderr, "%s: %s\n", prog_execname(), strerror(errno));
			return;
		}
		el->msgs[0]=0;
		el->msgs_len=0;
	}
			
	el->errors=TRUE;
	
	l=strlen(message)+1;
	
	if(l+el->msgs_len+1>ERRORLOG_MAX_SIZE){
		int n=0;
		if(l<ERRORLOG_MAX_SIZE){
			n=ERRORLOG_MAX_SIZE-l;
			memmove(el->msgs, el->msgs+ERRORLOG_MAX_SIZE-n, n);
			el->msgs[n]='\n';
		}
		memcpy(el->msgs+n, message+l-(ERRORLOG_MAX_SIZE-n),
			   ERRORLOG_MAX_SIZE-n);
	}else{
		if(el->msgs_len>0){
			el->msgs[el->msgs_len]='\n';
			strcpy(el->msgs+el->msgs_len+1, message);
		}else{
			strcpy(el->msgs, message);
		}
	}
	el->msgs_len=strlen(el->msgs);
}


static void log_warn_handler(const char *message)
{
	if(current_log!=NULL)
		add_error(current_log, message);
	
	/* Also print to stderr */
	fprintf(stderr, "%s: %s\n", prog_execname(), message);
}

		   
void begin_errorlog_file(ErrorLog *el, FILE *file)
{
	el->msgs=NULL;
	el->msgs_len=0;
	el->file=file;
	el->prev=current_log;
	el->errors=FALSE;
	el->old_handler=set_warn_handler(log_warn_handler);
	current_log=el;
}


void begin_errorlog(ErrorLog *el)
{
	begin_errorlog_file(el, NULL);
}


bool end_errorlog(ErrorLog *el)
{
	current_log=el->prev;
	set_warn_handler(el->old_handler);
	el->prev=NULL;
	el->old_handler=NULL;
	return el->errors;
}


void deinit_errorlog(ErrorLog *el)
{
	if(el->msgs!=NULL)
		free(el->msgs);
	el->msgs=NULL;
	el->msgs_len=0;
	el->file=NULL;
	el->errors=FALSE;
}

