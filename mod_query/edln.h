/*
 * ion/mod_query/edln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_EDLN_H
#define ION_MOD_QUERY_EDLN_H

#include <ioncore/common.h>
#include <libtu/obj.h>

INTRSTRUCT(Edln);

typedef void EdlnUpdateHandler(void*, int from, int mode);

#define EDLN_UPDATE_MOVED   0x01
#define EDLN_UPDATE_CHANGED 0x02
#define EDLN_UPDATE_NEW     0x04
        
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
    char *context;

    EdlnUpdateHandler *ui_update;
};


bool edln_insstr(Edln *edln, const char *str);
bool edln_insstr_n(Edln *edln, const char *str, int len,
                   bool update, bool movepoint);
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
void edln_history_prev(Edln *edln, bool match);
void edln_history_next(Edln *edln, bool match);
bool edln_set_context(Edln *edln, const char *str);

bool edln_init(Edln *edln, const char *dflt);
void edln_deinit(Edln *edln);
char* edln_finish(Edln *edln);

#endif /* ION_MOD_QUERY_EDLN_H */
