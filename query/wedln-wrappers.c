/*
 * ion/query/wedln-wrappers.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "wedln.h"
#include "edln.h"
#include "complete.h"

EXTL_EXPORT
void wedln_back(WEdln *wedln)
{
    edln_back(&(wedln->edln));
}

EXTL_EXPORT
void wedln_forward(WEdln *wedln)
{
    edln_forward(&(wedln->edln));
}

EXTL_EXPORT
void wedln_bol(WEdln *wedln)
{
    edln_bol(&(wedln->edln));
}

EXTL_EXPORT
void wedln_eol(WEdln *wedln)
{
    edln_eol(&(wedln->edln));
}

EXTL_EXPORT
void wedln_skip_word(WEdln *wedln)
{
    edln_skip_word(&(wedln->edln));
}

EXTL_EXPORT
void wedln_bskip_word(WEdln *wedln)
{
    edln_bskip_word(&(wedln->edln));
}

EXTL_EXPORT
void wedln_delete(WEdln *wedln)
{
    edln_delete(&(wedln->edln));
}

EXTL_EXPORT
void wedln_backspace(WEdln *wedln)
{
    edln_backspace(&(wedln->edln));
}

EXTL_EXPORT
void wedln_kill_to_eol(WEdln *wedln)
{
    edln_kill_to_eol(&(wedln->edln));
}

EXTL_EXPORT
void wedln_kill_to_bol(WEdln *wedln)
{
    edln_kill_to_bol(&(wedln->edln));
}

EXTL_EXPORT
void wedln_kill_line(WEdln *wedln)
{
    edln_kill_line(&(wedln->edln));
}

EXTL_EXPORT
void wedln_kill_word(WEdln *wedln)
{
    edln_kill_word(&(wedln->edln));
}

EXTL_EXPORT
void wedln_bkill_word(WEdln *wedln)
{
    edln_bkill_word(&(wedln->edln));
}

EXTL_EXPORT
void wedln_set_mark(WEdln *wedln)
{
    edln_set_mark(&(wedln->edln));
}

EXTL_EXPORT
void wedln_cut(WEdln *wedln)
{
    edln_cut(&(wedln->edln));
}

EXTL_EXPORT
void wedln_copy(WEdln *wedln)
{
    edln_copy(&(wedln->edln));
}

EXTL_EXPORT
void wedln_complete(WEdln *wedln)
{
    edln_complete(&(wedln->edln));
}

EXTL_EXPORT
void wedln_history_next(WEdln *wedln)
{
    edln_history_next(&(wedln->edln));
}

EXTL_EXPORT
void wedln_history_prev(WEdln *wedln)
{
    edln_history_prev(&(wedln->edln));
}

