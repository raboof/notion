/*
 * query/edln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_EDLN_H
#define QUERY_EDLN_H

#include <wmcore/common.h>
#include <wmcore/obj.h>

INTRSTRUCT(Edln)

typedef int EdlnCompletionHandler(char *p, char ***ret, char **beg,
								  void *data);
typedef void EdlnUpdateHandler(void*, int, bool moved);
typedef void EdlnShowComplHandler(void*, char **, int);
typedef void EdlnHideComplHandler(void*);


DECLSTRUCT(Edln){
	char *p;
	char *tmp_p;
	int point;
	int mark;
	int psize;
	int palloced;
	int tmp_palloced;
	int modified;
	int histent;
	void *uiptr;

	EdlnCompletionHandler *completion_handler;
	void *completion_handler_data;
	
	EdlnUpdateHandler *ui_update;
	EdlnShowComplHandler *ui_show_completions;
	EdlnHideComplHandler *ui_hide_completions;
};

	
bool edln_insch(Edln *edln, char ch);
bool edln_ovrch(Edln *edln, char ch);
bool edln_insstr(Edln *edln, const char *str);
bool edln_insstr_n(Edln *edln, const char *str, int len);
void edln_back(Edln *edln);
void edln_forward(Edln *edln);
void edln_bol(Edln *edln);
void edln_eol(Edln *edln);
void edln_bskip_word(Edln *edln);
void edln_skip_word(Edln *edln);
void edln_set_point(Edln *edln, int point);
void edln_delete(Edln *edln);
void edln_backspace(Edln *edln);
void edln_kill_to_eol(Edln *edln);
void edln_kill_to_bol(Edln *edln);
void edln_kill_line(Edln *edln);
void edln_kill_word(Edln *edln);
void edln_bkill_word(Edln *edln);
void edln_set_mark(Edln *edln);
void edln_clear_mark(Edln *edln);
void edln_cut(Edln *edln);
void edln_copy(Edln *edln);
void edln_history_prev(Edln *edln);
void edln_history_next(Edln *edln);

bool edln_init(Edln *edln, const char *dflt);
void edln_deinit(Edln *edln);
char* edln_finish(Edln *edln);

#endif /* QUERY_EDLN_H */
