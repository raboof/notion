/*
 * ion/query/wedln-wrappers.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "wedln.h"
#include "edln.h"
#include "complete.h"

/*EXTL_DOC
 * Move backward one character.
 */
EXTL_EXPORT_MEMBER
void wedln_back(WEdln *wedln)
{
    edln_back(&(wedln->edln));
}

/*EXTL_DOC
 * Move forward one character.
 */
EXTL_EXPORT_MEMBER
void wedln_forward(WEdln *wedln)
{
    edln_forward(&(wedln->edln));
}

/*EXTL_DOC
 * Go to the beginning of line.
 */
EXTL_EXPORT_MEMBER
void wedln_bol(WEdln *wedln)
{
    edln_bol(&(wedln->edln));
}

/*EXTL_DOC
 * Go to the end of line.
 */
EXTL_EXPORT_MEMBER
void wedln_eol(WEdln *wedln)
{
    edln_eol(&(wedln->edln));
}

/*EXTL_DOC
 * Go to to end of current sequence of whitespace followed by alphanumeric
 * characters..
 */
EXTL_EXPORT_MEMBER
void wedln_skip_word(WEdln *wedln)
{
    edln_skip_word(&(wedln->edln));
}

/*EXTL_DOC
 * Go to to beginning of current sequence of alphanumeric characters
 * followed by whitespace.
 */
EXTL_EXPORT_MEMBER
void wedln_bskip_word(WEdln *wedln)
{
    edln_bskip_word(&(wedln->edln));
}

/*EXTL_DOC
 * Delete current character.
 */
EXTL_EXPORT_MEMBER
void wedln_delete(WEdln *wedln)
{
    edln_delete(&(wedln->edln));
}

/*EXTL_DOC
 * Delete previous character.
 */
EXTL_EXPORT_MEMBER
void wedln_backspace(WEdln *wedln)
{
    edln_backspace(&(wedln->edln));
}

/*EXTL_DOC
 * Delete all characters from current to end of line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_to_eol(WEdln *wedln)
{
    edln_kill_to_eol(&(wedln->edln));
}

/*EXTL_DOC
 * Delete all characters from previous to beginning of line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_to_bol(WEdln *wedln)
{
    edln_kill_to_bol(&(wedln->edln));
}

/*EXTL_DOC
 * Delete the whole line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_line(WEdln *wedln)
{
    edln_kill_line(&(wedln->edln));
}

/*EXTL_DOC
 * Starting from the current point, delete possible whitespace and
 * following alphanumeric characters until next non-alphanumeric character.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_word(WEdln *wedln)
{
    edln_kill_word(&(wedln->edln));
}

/*EXTL_DOC
 * Starting from the previous characters, delete possible whitespace and
 * preceding alphanumeric characters until previous non-alphanumeric character.
 */
EXTL_EXPORT_MEMBER
void wedln_bkill_word(WEdln *wedln)
{
    edln_bkill_word(&(wedln->edln));
}

/*EXTL_DOC
 * Set \emph{mark} to current cursor position.
 */
EXTL_EXPORT_MEMBER
void wedln_set_mark(WEdln *wedln)
{
    edln_set_mark(&(wedln->edln));
}

/*EXTL_DOC
 * Clear \emph{mark}.
 */
EXTL_EXPORT_MEMBER
void wedln_clear_mark(WEdln *wedln)
{
    edln_clear_mark(&(wedln->edln));
}

/*EXTL_DOC
 * Copy text between \emph{mark} and current cursor position to clipboard
 * and then delete that sequence.
 */
EXTL_EXPORT_MEMBER
void wedln_cut(WEdln *wedln)
{
    edln_cut(&(wedln->edln));
}

/*EXTL_DOC
 * Copy text between \emph{mark} and current cursor position to clipboard.
 */
EXTL_EXPORT_MEMBER
void wedln_copy(WEdln *wedln)
{
    edln_copy(&(wedln->edln));
}

/*EXTL_DOC
 * Call completion handler with the text between the beginning of line and
 * current cursor position.
 */
EXTL_EXPORT_MEMBER
void wedln_complete(WEdln *wedln)
{
    edln_complete(&(wedln->edln));
}

/*EXTL_DOC
 * Replace line editor contents with the entry in history if one exists.
 */
EXTL_EXPORT_MEMBER
void wedln_history_next(WEdln *wedln)
{
    edln_history_next(&(wedln->edln));
}

/*EXTL_DOC
 * Replace line editor contents with the previous in history if one exists.
 */
EXTL_EXPORT_MEMBER
void wedln_history_prev(WEdln *wedln)
{
    edln_history_prev(&(wedln->edln));
}


/*EXTL_DOC
 * Input \var{str} in wedln at current editing point.
 */
EXTL_EXPORT_MEMBER_AS(WEdln, insstr)
void wedln_insstr_exported(WEdln *wedln, const char *str)
{
	edln_insstr(&(wedln->edln), str);
}


/*EXTL_DOC
 * Get line editor contents.
 */
EXTL_EXPORT_MEMBER
const char *wedln_contents(WEdln *wedln)
{
	return wedln->edln.p;
}

/*EXTL_DOC
 * Get current editing point.
 */
EXTL_EXPORT_MEMBER
int wedln_point(WEdln *wedln)
{
	return wedln->edln.point;
}
