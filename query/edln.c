/*
 * wmcore/edln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <ctype.h>
#include <string.h>

#include <wmcore/common.h>
#include <wmcore/selection.h>

#include "edln.h"
#include "wedln.h"

#define EDLN_ALLOCUNIT 16
#define EDLN_HISTORY_SIZE 256


#define UPDATE(X) edln->ui_update(edln->uiptr, X, FALSE)
#define UPDATE_MOVED(X) edln->ui_update(edln->uiptr, X, TRUE)


/*{{{ Alloc */


static bool edln_pspc(Edln *edln, int n)
{
	char *np;
	int pa;
	
	if(edln->palloced<edln->psize+1+n){
		pa=edln->palloced+n;
		pa|=(EDLN_ALLOCUNIT-1);
		np=ALLOC_N(char, pa);
		
		if(np==NULL){
			warn_err();
			return FALSE;
		}
		memmove(np, edln->p, edln->point*sizeof(char));
		memmove(np+edln->point+n, edln->p+edln->point,
				(edln->psize-edln->point+1)*sizeof(char));
		free(edln->p);
		edln->p=np;
		edln->palloced=pa;
	}else{
		memmove(edln->p+edln->point+n, edln->p+edln->point,
				(edln->psize-edln->point+1)*sizeof(char));
	}

	edln->psize+=n;

	edln->modified=1;
	return TRUE;
}


static bool edln_rspc(Edln *edln, int n)
{
	char *np;
	int pa;
	
	if(n+edln->point>=edln->psize)
		n=edln->psize-edln->point;
	
	if(n==0)
		return TRUE;
	
	if((edln->psize+1-n)<(edln->palloced&~(EDLN_ALLOCUNIT-1))){
		pa=edln->palloced&~(EDLN_ALLOCUNIT-1);
		np=ALLOC_N(char, pa);
		
		if(np==NULL){
			warn_err();
			goto norm;
		}
		
		memmove(np, edln->p, edln->point*sizeof(char));
		memmove(np+edln->point, edln->p+edln->point+n,
				(edln->psize-edln->point+1-n)*sizeof(char));
		free(edln->p);
		edln->p=np;
		edln->palloced=pa;
	}else{
	norm:
		memmove(edln->p+edln->point, edln->p+edln->point+n,
				(edln->psize-edln->point+1-n)*sizeof(char));
	}
	edln->psize-=n;
	
	edln->modified=1;
	return TRUE;
}


static void edln_clearstr(Edln *edln)
{
	if(edln->p!=NULL)
		free(edln->p);
	edln->palloced=0;
	edln->psize=0;
}


static bool edln_initstr(Edln *edln, const char *p)
{
	int l=strlen(p), al;
	
	al=(l+1)|(EDLN_ALLOCUNIT-1);
	
	edln->p=ALLOC_N(char, al);
	
	if(edln->p==NULL){
		warn_err();
		return FALSE;
	}

	edln->palloced=al;
	edln->psize=l;
	strcpy(edln->p, p);
	
	return TRUE;
}


static bool edln_setstr(Edln *edln, const char *p)
{
	edln_clearstr(edln);
	return edln_initstr(edln, p);
}


/*}}}*/


/*{{{ Insert */


bool edln_insch(Edln *edln, char ch)
{
	if(edln_pspc(edln, 1)){
		edln->p[edln->point]=ch;
		edln->point++;
		UPDATE_MOVED(edln->point-1);
		return TRUE;
	}
	return FALSE;
}


bool edln_ovrch(Edln *edln, char ch)
{
	edln_delete(edln);
	return edln_insch(edln, ch);
}


bool edln_insstr(Edln *edln, const char *str)
{
	int l;
	
	if(str==NULL)
		return FALSE;
	
	l=strlen(str);
	
	return edln_insstr_n(edln, str, l);
}


bool edln_insstr_n(Edln *edln, const char *str, int l)
{
	if(!edln_pspc(edln, l))
		return FALSE;
	
	memmove(&(edln->p[edln->point]), str, l);
	edln->point+=l;
	UPDATE_MOVED(edln->point-l);

	return TRUE;
}


/*}}}*/


/*{{{ Movement */


void edln_back(Edln *edln)
{
	if(edln->point>0){
		edln->point--;
		UPDATE_MOVED(edln->point);
	}
}


void edln_forward(Edln *edln)
{
	if(edln->point<edln->psize){
		edln->point++;
		UPDATE_MOVED(edln->point-1);
	}
}


void edln_bol(Edln *edln)
{
	if(edln->point!=0){
		edln->point=0;
		UPDATE_MOVED(0);
	}
}


void edln_eol(Edln *edln)
{
	int o=edln->point;
	
	if(edln->point!=edln->psize){
		edln->point=edln->psize;
		UPDATE_MOVED(o);
	}
}


void edln_bskip_word(Edln *edln)
{
	int p=edln->point-1;

	while(p>0){
		if(isalnum(edln->p[p]))
			goto fnd;
		p--;
	}
	return;
	
fnd:
	while(p>0){
		if(!isalnum(edln->p[p])){
			p++;
			break;
		}
		p--;
	}
	edln->point=p;
	UPDATE_MOVED(edln->point);
}


void edln_skip_word(Edln *edln)
{
	int p=edln->point;
	int o=p;
	
	while(p<edln->psize){
		if(!isalnum(edln->p[p]))
			goto fnd;
		p++;
	}
	return;
	
fnd:
	while(p<edln->psize){
		if(isalnum(edln->p[p]))
			break;
		p++;
	}
	edln->point=p;
	UPDATE_MOVED(o);
}


void edln_set_point(Edln *edln, int point)
{
	int o=edln->point;
	
	if(point<0)
		point=0;
	else if(point>edln->psize)
		point=edln->psize;
	
	edln->point=point;
	
	if(o<point)
		UPDATE_MOVED(o);
	else
		UPDATE_MOVED(point);
}

			   
/*}}}*/


/*{{{ Delete */


void edln_delete(Edln *edln)
{
	edln_rspc(edln, 1);
	UPDATE(edln->point);
}


void edln_backspace(Edln *edln)
{
	if(edln->point!=0){
		edln_back(edln);
		edln_delete(edln);
		UPDATE_MOVED(edln->point);
	}
}

void edln_kill_to_eol(Edln *edln)
{
	edln_rspc(edln, edln->psize-edln->point);
	UPDATE(edln->point);
}


void edln_kill_to_bol(Edln *edln)
{
	int p=edln->point;
	
	edln_bol(edln);
	edln_rspc(edln, p);
	edln->point=0;
	UPDATE_MOVED(0);
}


void edln_kill_line(Edln *edln)
{
	edln_bol(edln);
	edln_kill_to_eol(edln);
	UPDATE_MOVED(0);
}


void edln_kill_word(Edln *edln)
{
	int p=edln->point;
	int o=p;
	
	if(!isalnum(edln->p[p])){
		while(p<edln->psize){
			p++;
			if(isalnum(edln->p[p]))
				break;
		}
	}else{
		while(p<edln->psize){
			p++;
			if(!isalnum(edln->p[p]))
				break;
		}
	}
	edln_rspc(edln, p-edln->point);
	UPDATE(o);
}


void edln_bkill_word(Edln *edln)
{
	int p=edln->point, p2;
	
	if(p==0)
		return;
	
	p--;
	if(!isalnum(edln->p[p])){
		while(p>0){
			p--;
			if(isalnum(edln->p[p])){
				p++;
				break;
			}
		}
	}else{
		while(p>0){
			p--;
			if(!isalnum(edln->p[p])){
				p++;
				break;
			}
		}
	}
	
	p2=edln->point;
	edln->point=p;
	edln_rspc(edln, p2-p);
	UPDATE_MOVED(edln->point);
}


/*}}}*/


/*{{{ Selection */


void edln_set_mark(Edln *edln)
{
	edln->mark=edln->point;
}


void edln_clear_mark(Edln *edln)
{
	int m=edln->mark;
	edln->mark=-1;
	
	if(m<edln->point)
		UPDATE(m);
	else
		UPDATE(edln->point);
}


static void edln_do_copy(Edln *edln, bool del)
{
	int beg, end;
	
	if(edln->mark<0 || edln->point==edln->mark)
		return;
	
	if(edln->point<edln->mark){
		beg=edln->point;
		end=edln->mark;
	}else{
		beg=edln->mark;
		end=edln->point;
	}
	
	set_selection(edln->p+beg, end-beg);
	
	if(del){
		edln->point=beg;
		edln_rspc(edln, end-beg);
	}
	edln->mark=-1;
	
	UPDATE(beg);
}


void edln_cut(Edln *edln)
{
	edln_do_copy(edln, TRUE);
}


void edln_copy(Edln *edln)
{
	edln_do_copy(edln, FALSE);
}


/*}}}*/


/*{{{ History */


static int hist_head=EDLN_HISTORY_SIZE;
static int hist_count=0;
static char *hist[EDLN_HISTORY_SIZE];


void edlnhist_push(const char *str)
{
	char *strc;
	
	if(hist_count>0 && strcmp(hist[hist_head], str)==0)
		return;
	
	strc=scopy(str);
	
	if(strc==NULL){
		warn_err();
		return;
	}
	
	hist_head--;
	if(hist_head<0)
		hist_head=EDLN_HISTORY_SIZE-1;
	
	if(hist_count==EDLN_HISTORY_SIZE)
		free(hist[hist_head]);
	else
		hist_count++;
	
	hist[hist_head]=strc;
}


void edlnhist_clear()
{
	while(hist_count!=0){
		free(hist[hist_head]);
		hist_count--;
		if(++hist_head==EDLN_HISTORY_SIZE)
			hist_head=0;
	}
	hist_head=EDLN_HISTORY_SIZE;
}


static void edln_do_set_hist(Edln *edln, int e)
{
	edln->histent=e;
	edln_setstr(edln, hist[e]);
	edln->point=edln->psize;
	edln->mark=-1;
	edln->modified=FALSE;
	UPDATE_MOVED(0);
}


void edln_history_prev(Edln *edln)
{
	int e;
	
	if(edln->histent==-1){
		if(hist_count==0)
			return;
		edln->tmp_p=edln->p;
		edln->p=NULL;
		e=hist_head;
	}else{
		e=edln->histent;
		if(e==(hist_head+hist_count-1)%EDLN_HISTORY_SIZE)
			return;
		e=(e+1)%EDLN_HISTORY_SIZE;
	}
	
	edln_do_set_hist(edln, e);
}


void edln_history_next(Edln *edln)
{
	int e=edln->histent;
	
	if(edln->histent==-1)
		return;
	
	if(edln->histent==hist_head){
		edln->histent=-1;
		if(edln->p!=NULL)
			free(edln->p);
		edln->p=edln->tmp_p;
		edln->tmp_p=NULL;
		edln->point=strlen(edln->p);
		edln->mark=-1;
		edln->modified=TRUE;
		UPDATE_MOVED(0);
		return;
	}
	
/*	e=edln->histent-1;*/

	edln_do_set_hist(edln, (e+EDLN_HISTORY_SIZE-1)%EDLN_HISTORY_SIZE);
}


/*}}}*/


/*{{{ Init/deinit */


bool edln_init(Edln *edln, const char *p)
{
	if(p==NULL)
		p="";
	
	if(!edln_initstr(edln, p))
		return FALSE;
	
	edln->point=edln->psize;
	edln->mark=-1;
	edln->histent=-1;
	edln->modified=FALSE;
	edln->completion_handler=NULL;
	edln->completion_handler_data=NULL;
	edln->tmp_p=NULL;
	
	return TRUE;
}


void edln_deinit(Edln *edln)
{
	if(edln->p!=NULL){
		free(edln->p);
		edln->p=NULL;
	}
	if(edln->tmp_p!=NULL){
		free(edln->tmp_p);
		edln->tmp_p=NULL;
	}
}


char* edln_finish(Edln *edln)
{
	char *p=edln->p;
	
	/*if(edln->modified)*/
	edlnhist_push(p);
	
	edln->p=NULL;
	edln->psize=edln->palloced=0;
	
	stripws(p);
	return p;
}

/*}}}*/

